#include "ble_adv_light.h"

#include "esphome/core/log.h"

#include <cmath>

namespace esphome::ble_adv_controller {

static const char *const TAG = "ble_adv_controller.light";

namespace {
float clamp_unit(float value) { return std::max(0.0F, std::min(1.0F, value)); }
}  // namespace

void BleAdvLight::set_traits(float cold_white_temperature, float warm_white_temperature) {
  this->traits_.set_supported_color_modes({light::ColorMode::COLD_WARM_WHITE});
  this->traits_.set_min_mireds(cold_white_temperature);
  this->traits_.set_max_mireds(warm_white_temperature);
}

void BleAdvLight::dump_config() {
  ESP_LOGCONFIG(TAG, "BLE advertising light:");
  ESP_LOGCONFIG(TAG, "  Cold white: %.1f mireds", this->traits_.get_min_mireds());
  ESP_LOGCONFIG(TAG, "  Warm white: %.1f mireds", this->traits_.get_max_mireds());
  ESP_LOGCONFIG(TAG, "  Minimum brightness: %.0f%%", this->min_brightness_ * 100.0F);
}

void BleAdvLight::setup_state(light::LightState *state) {
  this->state_ = state;
  ESP_LOGCONFIG(TAG, "Linked to light '%s'", state->get_name().c_str());
}

void BleAdvLight::write_state(light::LightState *state) {
  ESP_LOGD(TAG, "write_state on=%s br=%.0f%%", state->current_values.is_on() ? "true" : "false",
           state->current_values.get_brightness() * 100.0F);
  if (!state->current_values.is_on()) {
    if (!this->is_off_) {
      ESP_LOGD(TAG, "Switch OFF");
      this->command(CommandType::LIGHT_OFF);
    }
    this->is_off_ = true;
    this->brightness_ = 0;
    this->warm_color_ = 0;
    return;
  }

  const bool was_off = this->is_off_;
  if (was_off) {
    ESP_LOGD(TAG, "Switch ON");
    this->command(CommandType::LIGHT_ON);
    this->is_off_ = false;
  }

  const float brightness =
      clamp_unit(this->min_brightness_ + state->current_values.get_brightness() * (1.0F - this->min_brightness_));
  const float temperature_range = this->traits_.get_max_mireds() - this->traits_.get_min_mireds();
  float warm = temperature_range == 0
                   ? 0
                   : clamp_unit((state->current_values.get_color_temperature() - this->traits_.get_min_mireds()) /
                                temperature_range);
  if (this->get_parent()->is_reversed())
    warm = 1.0F - warm;

  const float brightness_diff = std::fabs(this->brightness_ - brightness) * 100.0F;
  const float temperature_diff = std::fabs(this->warm_color_ - warm) * 100.0F;
  const bool final_update = state->current_values == state->remote_values;
  if (!was_off &&
      ((brightness_diff < 3 && temperature_diff < 3 && !final_update) ||
       (final_update && brightness_diff == 0 && temperature_diff == 0))) {
    ESP_LOGV(TAG, "Skipping update (br diff %.1f%%, ct diff %.1f%%)", brightness_diff, temperature_diff);
    return;
  }
  this->brightness_ = brightness;
  this->warm_color_ = warm;

  if (this->get_parent()->supports(CommandType::LIGHT_WCOLOR) && !this->split_dim_cct_) {
    float cold_white;
    float warm_white;
    if (this->constant_brightness_) {
      state->current_values_as_cwww(&cold_white, &warm_white, true);
      const float target_level = state->current_values.get_state() * brightness;
      const float current_level = state->current_values.get_state() * state->current_values.get_brightness();
      if (current_level > 0.0F) {
        const float scale = target_level / current_level;
        cold_white *= scale;
        warm_white *= scale;
      }
    } else if (state->current_values.get_color_mode() & light::ColorCapability::COLD_WARM_WHITE) {
      light::LightColorValues values = state->current_values;
      values.set_brightness(brightness);
      if (this->get_parent()->is_reversed())
        values.as_cwww(&warm_white, &cold_white, false);
      else
        values.as_cwww(&cold_white, &warm_white, false);
      cold_white = state->gamma_correct_lut(cold_white);
      warm_white = state->gamma_correct_lut(warm_white);
    } else {
      cold_white = (1.0F - warm) * brightness;
      warm_white = warm * brightness;
      cold_white = state->gamma_correct_lut(cold_white);
      warm_white = state->gamma_correct_lut(warm_white);
    }
    ESP_LOGD(TAG, "LIGHT_WCOLOR cold=%.0f%% warm=%.0f%%", cold_white * 100.0F, warm_white * 100.0F);
    this->command(CommandType::LIGHT_WCOLOR, static_cast<uint8_t>(cold_white * 255.0F),
                  static_cast<uint8_t>(warm_white * 255.0F));
    return;
  }
  if (temperature_diff != 0 || was_off) {
    ESP_LOGD(TAG, "LIGHT_CCT warm=%.0f%%", warm * 100.0F);
    this->command(CommandType::LIGHT_CCT, static_cast<uint8_t>(warm * 255.0F));
  }
  if (brightness_diff != 0 || was_off) {
    ESP_LOGD(TAG, "LIGHT_DIM brightness=%.0f%%", brightness * 100.0F);
    this->command(CommandType::LIGHT_DIM, static_cast<uint8_t>(brightness * 255.0F));
  }
}

void BleAdvSecLight::dump_config() { ESP_LOGCONFIG(TAG, "BLE advertising secondary light"); }

void BleAdvSecLight::write_state(light::LightState *state) {
  bool enabled;
  state->current_values_as_binary(&enabled);
  this->command(enabled ? CommandType::LIGHT_SEC_ON : CommandType::LIGHT_SEC_OFF);
}

}  // namespace esphome::ble_adv_controller
