#include "ble_adv_controller.h"
#include "controller_logic.h"

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome::ble_adv_controller {

static const char *const TAG = "ble_adv_controller";

void BleAdvController::dump_config() {
  ESP_LOGCONFIG(TAG, "BLE advertising controller:");
  ESP_LOGCONFIG(TAG, "  Encoding: %s", this->encoding_.c_str());
  ESP_LOGCONFIG(TAG, "  Variant: %s", this->variant_.c_str());
  ESP_LOGCONFIG(TAG, "  ID: 0x%08" PRIX32, this->params_.id);
  ESP_LOGCONFIG(TAG, "  Index: %u", this->params_.index);
  ESP_LOGCONFIG(TAG, "  Minimum duration: %" PRIu32 " ms", this->min_duration_);
  ESP_LOGCONFIG(TAG, "  Maximum duration: %" PRIu32 " ms", this->max_duration_);
  ESP_LOGCONFIG(TAG, "  Sequence duration: %" PRIu32 " ms", this->sequence_duration_);
}

bool BleAdvController::supports(CommandType type) const {
  if (this->handler_ == nullptr)
    return false;
  return this->handler_->registry().supports(this->encoding_, this->variant_, Command(type));
}

bool BleAdvController::enqueue(Command command) {
  if (this->handler_ == nullptr) {
    ESP_LOGE(TAG, "BLE handler is not configured");
    return false;
  }
  if (!this->handler_->registry().supports(this->encoding_, this->variant_, command)) {
    ESP_LOGW(TAG, "Unsupported command %u for %s/%s", static_cast<unsigned>(command.type), this->encoding_.c_str(),
             this->variant_.c_str());
    return false;
  }
  this->params_.tx_count = logic::normalize_tx_count(this->params_.tx_count);
  auto packets = this->handler_->registry().encode(this->encoding_, this->variant_, command, this->params_);
  if (packets.empty())
    return false;
  const bool sequence = this->sequence_duration_ > 0 && this->sequence_duration_ < this->min_duration_;
  for (auto &packet : packets)
    packet.min_duration_ms = sequence ? this->sequence_duration_ : this->min_duration_;
  if (logic::should_replace_queued_command(command.type)) {
    this->queue_.remove_if([&](const QueueItem &item) { return item.type == command.type; });
  }
  this->queue_.push_back({command.type, std::move(packets)});
  return true;
}

bool BleAdvController::inject_raw(const std::string &raw) {
  protocol::AdvPacket packet;
  if (!packet.from_hex(raw)) {
    ESP_LOGE(TAG, "Invalid raw packet");
    return false;
  }
  packet.min_duration_ms = this->min_duration_;
  this->queue_.push_back({CommandType::CUSTOM, {packet}});
  return true;
}

bool BleAdvController::decode_raw(const std::string &raw) {
  return this->handler_ != nullptr && this->handler_->decode_hex_and_log(raw);
}

void BleAdvController::loop() {
  const uint32_t now = millis();
  if (this->active_message_id_ == 0) {
    if (this->queue_.empty())
      return;
    this->active_message_id_ = this->handler_->add_packets(std::move(this->queue_.front().packets));
    this->queue_.pop_front();
    this->active_started_at_ = now;
    return;
  }
  const uint32_t duration =
      logic::message_duration(!this->queue_.empty(), this->min_duration_, this->max_duration_);
  if (now - this->active_started_at_ >= duration) {
    this->handler_->remove_packets(this->active_message_id_);
    this->active_message_id_ = 0;
    this->active_started_at_ = 0;
  }
}

bool BleAdvEntity::command(CommandType type, uint8_t arg0, uint8_t arg1) {
  Command command(type);
  command.args[0] = arg0;
  command.args[1] = arg1;
  return this->get_parent()->enqueue(command);
}

bool BleAdvEntity::command(CommandType type, const std::vector<uint8_t> &args) {
  Command command(type);
  std::copy(args.begin(), args.begin() + std::min(args.size(), command.args.size()), command.args.begin());
  return this->get_parent()->enqueue(command);
}

}  // namespace esphome::ble_adv_controller
