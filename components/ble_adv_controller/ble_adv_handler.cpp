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

void BleAdvHandler::configure_front_() {
  if (this->packets_.empty() || this->state_ != AdvertiserState::IDLE)
    return;
  auto &packet = this->packets_.front().packet;
  this->last_packet_hex_ = packet.to_hex();
  this->owns_advertiser_ = true;
  const esp_err_t error =
      esp_ble_gap_config_adv_data_raw(const_cast<uint8_t *>(packet.bytes.data()), static_cast<uint32_t>(packet.len));
  if (error != ESP_OK) {
    ESP_LOGE(TAG, "esp_ble_gap_config_adv_data_raw failed: %s", esp_err_to_name(error));
    this->packets_.pop_front();
    return;
  }
  this->state_ = AdvertiserState::CONFIGURING;
}

void BleAdvHandler::acquire_advertiser_() {
  const esp_err_t error = esp_ble_gap_stop_advertising();
  if (error == ESP_OK) {
    this->state_ = AdvertiserState::ACQUIRING;
    return;
  }
  ESP_LOGV(TAG, "Advertising was already stopped: %s", esp_err_to_name(error));
  this->owns_advertiser_ = true;
  this->configure_front_();
}

void BleAdvHandler::request_stop_() {
  if (this->state_ != AdvertiserState::ADVERTISING)
    return;
  const esp_err_t error = esp_ble_gap_stop_advertising();
  if (error != ESP_OK) {
    ESP_LOGE(TAG, "esp_ble_gap_stop_advertising failed: %s", esp_err_to_name(error));
    this->finish_front_();
    return;
  }
  this->state_ = AdvertiserState::STOPPING;
}

void BleAdvHandler::finish_front_() {
  if (this->packets_.empty()) {
    this->state_ = AdvertiserState::IDLE;
    return;
  }
  auto current = std::move(this->packets_.front());
  this->packets_.pop_front();
  if (!current.remove)
    this->packets_.push_back(std::move(current));
  this->state_ = AdvertiserState::IDLE;
  this->packet_started_at_ = 0;
}

void BleAdvHandler::restore_esphome_advertising_() {
  if (!this->owns_advertiser_ || this->ble_parent_ == nullptr)
    return;
  this->owns_advertiser_ = false;
  this->ble_parent_->advertising_start();
}

void BleAdvHandler::loop() {
  if (this->is_failed())
    return;
  if (this->state_ == AdvertiserState::IDLE) {
    this->packets_.remove_if([](const ScheduledPacket &packet) { return packet.processed && packet.remove; });
    if (this->packets_.empty()) {
      this->restore_esphome_advertising_();
      return;
    }
    if (!this->owns_advertiser_) {
      this->acquire_advertiser_();
      return;
    }
    this->configure_front_();
    return;
  }
  if (this->state_ != AdvertiserState::ADVERTISING || this->packets_.empty())
    return;
  const auto &front = this->packets_.front();
  const bool should_rotate = this->packets_.size() > 1 || front.remove;
  if (should_rotate && millis() - this->packet_started_at_ >= front.packet.min_duration_ms)
    this->request_stop_();
}

void BleAdvHandler::gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
  switch (event) {
    case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
      if (this->state_ != AdvertiserState::CONFIGURING)
        return;
      if (param->adv_data_raw_cmpl.status != ESP_BT_STATUS_SUCCESS) {
        ESP_LOGE(TAG, "Raw advertising configuration failed: %d", param->adv_data_raw_cmpl.status);
        this->finish_front_();
        return;
      }
      if (const esp_err_t error = esp_ble_gap_start_advertising(&this->advertising_params_); error != ESP_OK) {
        ESP_LOGE(TAG, "esp_ble_gap_start_advertising failed: %s", esp_err_to_name(error));
        this->finish_front_();
        return;
      }
      this->state_ = AdvertiserState::STARTING;
      break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
      if (this->state_ != AdvertiserState::STARTING)
        return;
      if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
        ESP_LOGE(TAG, "Advertising start failed: %d", param->adv_start_cmpl.status);
        this->finish_front_();
        return;
      }
      if (!this->packets_.empty())
        this->packets_.front().processed = true;
      this->packet_started_at_ = millis();
      this->state_ = AdvertiserState::ADVERTISING;
      break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
      if (this->state_ == AdvertiserState::ACQUIRING) {
        this->owns_advertiser_ = true;
        this->state_ = AdvertiserState::IDLE;
      } else if (this->state_ == AdvertiserState::STOPPING) {
        this->finish_front_();
      }
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
