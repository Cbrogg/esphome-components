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

bool BleAdvHandler::start_front_packet_() {
  if (this->packets_.empty())
    return false;

  const uint32_t now = millis();
  if (now - this->last_config_attempt_ < CONFIG_RETRY_MS)
    return false;

  this->last_config_attempt_ = now;
  auto &front = this->packets_.front();
  auto &packet = front.packet;
  this->last_packet_hex_ = packet.to_hex();

  esp_ble_gap_stop_advertising();

  const esp_err_t config_error = esp_ble_gap_config_adv_data_raw(
      const_cast<uint8_t *>(packet.bytes.data()), static_cast<uint32_t>(packet.len));
  if (config_error != ESP_OK) {
    ESP_LOGE(TAG, "esp_ble_gap_config_adv_data_raw failed: %s", esp_err_to_name(config_error));
    if (++front.config_failures >= MAX_CONFIG_FAILURES) {
      ESP_LOGE(TAG, "Dropping packet after %u config failures: %s", front.config_failures,
               packet.to_hex().c_str());
      this->packets_.pop_front();
    }
    return false;
  }

  const esp_err_t start_error = esp_ble_gap_start_advertising(&this->advertising_params_);
  if (start_error != ESP_OK) {
    ESP_LOGE(TAG, "esp_ble_gap_start_advertising failed: %s", esp_err_to_name(start_error));
    if (++front.config_failures >= MAX_CONFIG_FAILURES) {
      ESP_LOGE(TAG, "Dropping packet after %u start failures: %s", front.config_failures,
               packet.to_hex().c_str());
      this->packets_.pop_front();
    }
    return false;
  }

  front.processed = true;
  front.config_failures = 0;
  this->adv_active_until_ = now + packet.min_duration_ms;
  ESP_LOGD(TAG, "Advertising packet for %u ms", packet.min_duration_ms);
  return true;
}

void BleAdvHandler::rotate_after_stop_() {
  if (this->packets_.empty()) {
    this->adv_active_until_ = 0;
    this->stop_settle_until_ = 0;
    return;
  }

  auto &front = this->packets_.front();
  if (front.remove) {
    this->packets_.pop_front();
  } else if (this->packets_.size() > 1) {
    this->packets_.push_back(std::move(front));
    this->packets_.pop_front();
  }

  this->adv_active_until_ = 0;
  this->stop_settle_until_ = millis() + STOP_SETTLE_MS;
}

void BleAdvHandler::loop() {
  if (this->is_failed())
    return;

  this->packets_.remove_if([](const ScheduledPacket &packet) { return packet.processed && packet.remove; });

  if (this->packets_.empty()) {
    this->adv_active_until_ = 0;
    this->stop_settle_until_ = 0;
    return;
  }

  const uint32_t now = millis();

  if (this->stop_settle_until_ != 0) {
    if (now < this->stop_settle_until_)
      return;
    this->stop_settle_until_ = 0;
    this->start_front_packet_();
    return;
  }

  if (this->adv_active_until_ != 0) {
    if (now < this->adv_active_until_)
      return;

    const auto &front = this->packets_.front();
    const bool should_rotate = this->packets_.size() > 1 || front.remove;
    if (!should_rotate) {
      this->adv_active_until_ = now + front.packet.min_duration_ms;
      return;
    }

    esp_ble_gap_stop_advertising();
    this->rotate_after_stop_();
    return;
  }

  this->start_front_packet_();
}

void BleAdvHandler::gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
  switch (event) {
    case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
      if (param->adv_data_raw_cmpl.status != ESP_BT_STATUS_SUCCESS) {
        ESP_LOGW(TAG, "Raw advertising configuration async status: %d", param->adv_data_raw_cmpl.status);
      }
      break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
      if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
        ESP_LOGW(TAG, "Advertising async start status: %d", param->adv_start_cmpl.status);
      }
      break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
      if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
        ESP_LOGW(TAG, "Advertising async stop status: %d", param->adv_stop_cmpl.status);
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
