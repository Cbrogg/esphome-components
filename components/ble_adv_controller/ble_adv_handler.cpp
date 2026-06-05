#include "ble_adv_handler.h"

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome::ble_adv_controller {

static const char *const TAG = "ble_adv_controller.hub";

void BleAdvHandler::setup() {
  if (this->ble_parent_ == nullptr) {
    ESP_LOGE(TAG, "ESP32 BLE parent is not configured");
    this->mark_failed();
  }
}

void BleAdvHandler::dump_config() {
  ESP_LOGCONFIG(TAG, "BLE advertising hub:");
  ESP_LOGCONFIG(TAG, "  Protocol encoders: %u", static_cast<unsigned>(this->registry_.encoders().size()));
}

bool BleAdvHandler::ble_ready_() const {
  return this->ble_parent_ != nullptr && this->ble_parent_->is_active();
}

uint16_t BleAdvHandler::add_packets(std::vector<protocol::AdvPacket> packets) {
  if (packets.empty())
    return 0;
  if (++this->next_message_id_ == 0)
    ++this->next_message_id_;
  const uint16_t message_id = this->next_message_id_;
  for (auto &packet : packets) {
    this->last_packet_hex_ = packet.to_hex();
    ESP_LOGD(TAG, "Queue packet %u: %s", message_id, this->last_packet_hex_.c_str());
    this->packets_.emplace_back(message_id, std::move(packet));
  }
  return message_id;
}

void BleAdvHandler::remove_packets(uint16_t message_id) {
  if (message_id == 0)
    return;
  for (auto &scheduled : this->packets_) {
    if (scheduled.message_id == message_id)
      scheduled.remove = true;
  }
}

void BleAdvHandler::begin_transmission_() {
  if (this->packets_.empty() || !this->ble_ready_())
    return;

  this->stop_requested_at_ = millis();
  const esp_err_t error = esp_ble_gap_stop_advertising();
  this->stop_was_running_ = (error == ESP_OK);
  this->state_ = AdvertiserState::STOPPING;
  if (!this->stop_was_running_) {
    ESP_LOGV(TAG, "GAP stop returned %s, waiting for idle", esp_err_to_name(error));
  }
}

void BleAdvHandler::begin_config_() {
  if (this->state_ != AdvertiserState::STOPPING) {
    return;
  }
  if (this->packets_.empty()) {
    this->state_ = AdvertiserState::IDLE;
    return;
  }

  auto &front = this->packets_.front();
  auto &packet = front.packet;
  this->last_packet_hex_ = packet.to_hex();

  const esp_err_t error = esp_ble_gap_config_adv_data_raw(
      const_cast<uint8_t *>(packet.bytes.data()), static_cast<uint32_t>(packet.len));
  if (error != ESP_OK) {
    ESP_LOGE(TAG, "esp_ble_gap_config_adv_data_raw failed: %s", esp_err_to_name(error));
    this->handle_failure_();
    return;
  }

  this->state_ = AdvertiserState::CONFIGURING;
}

void BleAdvHandler::begin_start_() {
  if (this->state_ != AdvertiserState::CONFIGURING) {
    return;
  }
  const esp_err_t error = esp_ble_gap_start_advertising(&this->advertising_params_);
  if (error != ESP_OK) {
    ESP_LOGE(TAG, "esp_ble_gap_start_advertising failed: %s", esp_err_to_name(error));
    this->handle_failure_();
    return;
  }
  this->state_ = AdvertiserState::STARTING;
}

void BleAdvHandler::handle_failure_() {
  this->state_ = AdvertiserState::IDLE;
  this->retry_after_ = millis() + RETRY_BACKOFF_MS;

  if (this->packets_.empty())
    return;

  auto &front = this->packets_.front();
  if (++front.config_failures >= MAX_CONFIG_FAILURES) {
    ESP_LOGE(TAG, "Dropping packet after %u failures: %s", front.config_failures, front.packet.to_hex().c_str());
    this->packets_.pop_front();
    front.config_failures = 0;
  }
}

void BleAdvHandler::request_rotate_stop_() {
  const esp_err_t error = esp_ble_gap_stop_advertising();
  this->stop_requested_at_ = millis();
  this->stop_was_running_ = (error == ESP_OK);
  this->state_ = AdvertiserState::STOPPING_ROTATE;
  if (!this->stop_was_running_) {
    this->finish_rotate_();
  }
}

void BleAdvHandler::finish_rotate_() {
  if (this->packets_.empty()) {
    this->state_ = AdvertiserState::IDLE;
    return;
  }

  auto &front = this->packets_.front();
  if (front.remove) {
    this->packets_.pop_front();
  } else if (this->packets_.size() > 1) {
    this->packets_.push_back(std::move(front));
    this->packets_.pop_front();
  }

  this->state_ = AdvertiserState::IDLE;
}

void BleAdvHandler::loop() {
  if (this->is_failed() || !this->ble_ready_())
    return;

  const uint32_t now = millis();

  if (this->state_ == AdvertiserState::IDLE) {
    this->packets_.remove_if([](const ScheduledPacket &packet) { return packet.processed && packet.remove; });
    if (this->packets_.empty())
      return;
    if (now < this->retry_after_)
      return;
    this->begin_transmission_();
    return;
  }

  if (this->state_ == AdvertiserState::STOPPING) {
    if (!this->stop_was_running_ && now - this->stop_requested_at_ >= STOP_IDLE_SETTLE_MS) {
      this->begin_config_();
      return;
    }
    if (this->stop_was_running_ && now - this->stop_requested_at_ >= STOP_EVENT_TIMEOUT_MS) {
      ESP_LOGW(TAG, "ADV_STOP_COMPLETE timeout, forcing config");
      this->begin_config_();
    }
    return;
  }

  if (this->state_ == AdvertiserState::STOPPING_ROTATE) {
    if (!this->stop_was_running_ && now - this->stop_requested_at_ >= STOP_IDLE_SETTLE_MS) {
      this->finish_rotate_();
    } else if (this->stop_was_running_ && now - this->stop_requested_at_ >= STOP_EVENT_TIMEOUT_MS) {
      ESP_LOGW(TAG, "ADV_STOP_COMPLETE timeout during rotate");
      this->finish_rotate_();
    }
    return;
  }

  if (this->state_ != AdvertiserState::ADVERTISING || this->packets_.empty())
    return;

  const auto &front = this->packets_.front();
  const bool should_rotate = this->packets_.size() > 1 || front.remove;
  if (!should_rotate) {
    if (now - this->packet_started_at_ >= front.packet.min_duration_ms)
      this->packet_started_at_ = now;
    return;
  }

  if (now - this->packet_started_at_ >= front.packet.min_duration_ms)
    this->request_rotate_stop_();
}

void BleAdvHandler::gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
  switch (event) {
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
      if (this->state_ == AdvertiserState::STOPPING) {
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
          ESP_LOGW(TAG, "Advertising stop status: %d", param->adv_stop_cmpl.status);
        }
        this->begin_config_();
      } else if (this->state_ == AdvertiserState::STOPPING_ROTATE) {
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
          ESP_LOGW(TAG, "Advertising stop status: %d", param->adv_stop_cmpl.status);
        }
        this->finish_rotate_();
      }
      break;

    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
      if (this->state_ == AdvertiserState::STOPPING && !this->stop_was_running_) {
        this->begin_config_();
      }
      break;

    case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
      if (this->state_ != AdvertiserState::CONFIGURING)
        return;
      if (param->adv_data_raw_cmpl.status != ESP_BT_STATUS_SUCCESS) {
        ESP_LOGE(TAG, "Raw advertising configuration failed: %d", param->adv_data_raw_cmpl.status);
        this->handle_failure_();
        return;
      }
      this->begin_start_();
      break;

    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
      if (this->state_ != AdvertiserState::STARTING)
        return;
      if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
        ESP_LOGE(TAG, "Advertising start failed: %d", param->adv_start_cmpl.status);
        this->handle_failure_();
        return;
      }
      if (!this->packets_.empty()) {
        this->packets_.front().processed = true;
        this->packets_.front().config_failures = 0;
      }
      this->packet_started_at_ = millis();
      this->state_ = AdvertiserState::ADVERTISING;
      ESP_LOGD(TAG, "Advertising started");
      break;

    default:
      break;
  }
}

bool BleAdvHandler::decode_and_log(const protocol::AdvPacket &packet, bool ignore_ble_params) {
  protocol::DecodeResult result;
  if (!this->registry_.decode(packet, result, ignore_ble_params)) {
    ESP_LOGW(TAG, "No encoder recognized packet: %s", packet.to_hex().c_str());
    return false;
  }
  ESP_LOGI(TAG, "Decoded %s/%s: id=0x%08" PRIX32 ", index=%u, tx=%u, cmd=0x%02X",
           result.encoder->encoding().c_str(), result.encoder->variant().c_str(), result.params.id,
           result.params.index, result.params.tx_count, result.command.raw_cmd);
  ESP_LOGI(TAG, "Suggested YAML:\nble_adv_controller:\n  - id: my_controller\n    encoding: %s\n    variant: %s\n"
                "    forced_id: 0x%08" PRIX32 "\n    index: %u",
           result.encoder->encoding().c_str(), result.encoder->variant().c_str(), result.params.id,
           result.params.index);
  if (result.roundtrip_equal) {
    ESP_LOGI(TAG, "Decode/re-encode: NO DIFF");
  } else {
    ESP_LOGW(TAG, "Decode/re-encode differs:\n  raw: %s\n  enc: %s", packet.to_hex().c_str(),
             result.reencoded.to_hex().c_str());
  }
  return true;
}

bool BleAdvHandler::decode_hex_and_log(const std::string &raw) {
  protocol::AdvPacket packet;
  if (!packet.from_hex(raw)) {
    ESP_LOGE(TAG, "Invalid raw advertising packet");
    return false;
  }
  return this->decode_and_log(packet);
}

}  // namespace esphome::ble_adv_controller
