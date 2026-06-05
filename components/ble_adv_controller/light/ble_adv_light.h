#pragma once

#include "../ble_adv_controller.h"
#include "esphome/components/light/light_output.h"
#include "esphome/core/component.h"

namespace esphome::ble_adv_controller {

class BleAdvLight : public light::LightOutput, public Parented<BleAdvController> {
 public:
  void set_traits(float cold_white_temperature, float warm_white_temperature);
  void set_constant_brightness(bool value) { this->constant_brightness_ = value; }
  void set_min_brightness(float value) { this->min_brightness_ = value; }
  void set_split_dim_cct(bool value) { this->split_dim_cct_ = value; }

  void setup_state(light::LightState *state) override;
  void update_state(light::LightState *state) override;
  void write_state(light::LightState *state) override;
  light::LightTraits get_traits() override { return this->traits_; }

 protected:
  bool send_command(CommandType type, uint8_t arg0 = 0, uint8_t arg1 = 0);

  light::LightState *state_{nullptr};
  light::LightTraits traits_;
  bool constant_brightness_{false};
  bool split_dim_cct_{false};
  float min_brightness_{0.01F};
  bool is_off_{true};
  float brightness_{0};
  float warm_color_{0};
};

class BleAdvSecLight : public light::LightOutput, public Parented<BleAdvController> {
 public:
  void set_traits() { this->traits_.set_supported_color_modes({light::ColorMode::ON_OFF}); }
  void setup_state(light::LightState *state) override;
  void write_state(light::LightState *state) override;
  light::LightTraits get_traits() override { return this->traits_; }

 protected:
  bool send_command(CommandType type, uint8_t arg0 = 0, uint8_t arg1 = 0);

  light::LightState *state_{nullptr};
  light::LightTraits traits_;
};

}  // namespace esphome::ble_adv_controller
