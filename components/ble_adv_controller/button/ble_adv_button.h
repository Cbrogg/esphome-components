#pragma once

#include "../ble_adv_controller.h"
#include "esphome/components/button/button.h"

namespace esphome::ble_adv_controller {

class BleAdvButton : public button::Button, public BleAdvEntity {
 public:
  void dump_config() override;
  void set_command(uint8_t command) { this->command_ = static_cast<CommandType>(command); }
  void set_args(std::vector<uint8_t> args) { this->args_ = std::move(args); }

 protected:
  void press_action() override;

  CommandType command_{CommandType::NOCMD};
  std::vector<uint8_t> args_;
};

}  // namespace esphome::ble_adv_controller
