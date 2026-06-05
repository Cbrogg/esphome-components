#include "ble_adv_button.h"

#include "esphome/core/log.h"

namespace esphome::ble_adv_controller {

static const char *const TAG = "ble_adv_controller.button";

void BleAdvButton::dump_config() { LOG_BUTTON("", "BLE advertising button", this); }

void BleAdvButton::press_action() {
  if (this->command_ != CommandType::CUSTOM) {
    this->command(this->command_, this->args_);
    return;
  }
  if (this->args_.size() != 5) {
    ESP_LOGE(TAG, "Custom command requires command byte and four arguments");
    return;
  }
  Command command(CommandType::CUSTOM);
  command.raw_cmd = this->args_[0];
  std::copy(this->args_.begin() + 1, this->args_.end(), command.args.begin());
  this->get_parent()->enqueue(command);
}

}  // namespace esphome::ble_adv_controller
