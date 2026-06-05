#include "ble_adv_light.h"

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

#include <cmath>

namespace esphome::ble_adv_controller {

static const char *const TAG = "ble_adv_controller.light";
static constexpr uint32_t APPLY_THROTTLE_MS = 200;

namespace {
float clamp_unit(float value) { return std::max(0.0F, std::min(1.0F, value)); }
}  // namespace

bool BleAdvLight::send_command(CommandType type, uint8_t arg0, uint8_t arg1) {
  if (this->parent_ == nullptr) {
    ESP_LOGE(TAG, "No controller parent — command %u dropped", static_cast<unsigned>(type));
    return false;
  }
  Command command(type);
  command.args[0] = arg0;
  command.args[1] = arg1;
  if (!this->parent_->enqueue(command)) {
    ESP_LOGW(TAG, "Enqueue failed for command %u", static_cast<unsigned>(type));
    return false;
  }
  return true;
}

bool BleAdvSecLight::send_command(CommandType type, uint8_t arg0, uint8_t arg1) {
  if (this->parent_ == nullptr) {
    ESP_LOGE(TAG, "No controller parent — command %u dropped", static_cast<unsigned>(type));
    return false;
  }
  Command command(type);
  command.args[0] = arg0;
  command.args[1] = arg1;
  return this->parent_->enqueue(command);
}

void BleAdvLight::set_traits(float cold_white_temperature, float warm_white_temperature) {
  this->traits_.set_supported_color_modes({light::ColorMode::COLD_WARM_WHITE});
  this->traits_.set_min_mireds(cold_white_temperature);
  this->traits_.set_max_mireds(warm_white_temperature);
}

void BleAdvLight::setup_state(light::LightState *state) {
  this->state_ = state;
  ESP_LOGI(TAG, "Ready: '%s' parent=%p cw=%.0f ww=%.0f min_br=%.0f%%", state->get_name().c_str(),
           static_cast<void *>(this->parent_), this->traits_.get_min_mireds(), this->traits_.get_max_mireds(),
           this->min_brightness_ * 100.0F);
}

void BleAdvLight::update_state(light::LightState *state) {
  ESP_LOGD(TAG, "update_state '%s' on=%s br=%.0f%%", state->get_name().c_str(),
           state->current_values.is_on() ? "true" : "false",
           state->current_values.get_brightness() * 100.0F);
  // HA/API applies changes via set_immediately_ → update_state; write_state may not run
  // if the light loop is idle. Apply BLE commands here (same pattern as addressable_light).
  this->apply_state(state);
}

void BleAdvLight::write_state(light::LightState *state) {
  // Hardware is driven from update_state(); loop may call write_state again — skip to avoid double BLE load.
  (void) state;
}

void BleAdvLight::apply_state(light::LightState *state) {
  const bool final_update = state->current_values == state->remote_values;

  if (!state->current_values.is_on()) {
    if (!this->is_off_) {
      ESP_LOGI(TAG, "Switch OFF");
      this->send_command(CommandType::LIGHT_OFF);
      this->last_apply_ms_ = millis();
    }
    this->is_off_ = true;
    this->brightness_ = 0;
    this->warm_color_ = 0;
    return;
  }

  const bool was_off = this->is_off_;
  if (was_off) {
    ESP_LOGI(TAG, "Switch ON");
    this->send_command(CommandType::LIGHT_ON);
    this->is_off_ = false;
    this->last_apply_ms_ = millis();
  }

  if (!was_off && !final_update) {
    const uint32_t now = millis();
    if (now - this->last_apply_ms_ < APPLY_THROTTLE_MS) {
      ESP_LOGV(TAG, "Throttled (%ums since last BLE)", now - this->last_apply_ms_);
      return;
    }
  }

  const float brightness =
      clamp_unit(this->min_brightness_ + state->current_values.get_brightness() * (1.0F - this->min_brightness_));
  const float temperature_range = this->traits_.get_max_mireds() - this->traits_.get_min_mireds();
  float warm = temperature_range == 0
                   ? 0
                   : clamp_unit((state->current_values.get_color_temperature() - this->traits_.get_min_mireds()) /
                                temperature_range);
  if (this->parent_ != nullptr && this->parent_->is_reversed())
    warm = 1.0F - warm;

  const float brightness_diff = std::fabs(this->brightness_ - brightness) * 100.0F;
  const float temperature_diff = std::fabs(this->warm_color_ - warm) * 100.0F;
  if (!was_off &&
      ((brightness_diff < 3 && temperature_diff < 3 && !final_update) ||
       (final_update && brightness_diff == 0 && temperature_diff == 0))) {
    ESP_LOGV(TAG, "Skipping update (br diff %.1f%%, ct diff %.1f%%)", brightness_diff, temperature_diff);
    return;
  }
  this->brightness_ = brightness;
  this->warm_color_ = warm;

  if (this->parent_ != nullptr && this->parent_->supports(CommandType::LIGHT_WCOLOR) && !this->split_dim_cct_) {
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
      if (this->parent_->is_reversed())
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
    ESP_LOGI(TAG, "LIGHT_WCOLOR cold=%.0f%% warm=%.0f%%", cold_white * 100.0F, warm_white * 100.0F);
    this->send_command(CommandType::LIGHT_WCOLOR, static_cast<uint8_t>(cold_white * 255.0F),
                       static_cast<uint8_t>(warm_white * 255.0F));
    this->last_apply_ms_ = millis();
    return;
  }
  if (temperature_diff != 0 || was_off) {
    ESP_LOGI(TAG, "LIGHT_CCT warm=%.0f%%", warm * 100.0F);
    this->send_command(CommandType::LIGHT_CCT, static_cast<uint8_t>(warm * 255.0F));
    this->last_apply_ms_ = millis();
  }
  if (brightness_diff != 0 || was_off) {
    ESP_LOGI(TAG, "LIGHT_DIM brightness=%.0f%%", brightness * 100.0F);
    this->send_command(CommandType::LIGHT_DIM, static_cast<uint8_t>(brightness * 255.0F));
    this->last_apply_ms_ = millis();
  }
}

void BleAdvSecLight::setup_state(light::LightState *state) {
  this->state_ = state;
  ESP_LOGCONFIG(TAG, "BLE advertising secondary light '%s', parent=%p", state->get_name().c_str(),
                static_cast<void *>(this->parent_));
}

void BleAdvSecLight::update_state(light::LightState *state) {
  bool enabled;
  state->current_values_as_binary(&enabled);
  this->send_command(enabled ? CommandType::LIGHT_SEC_ON : CommandType::LIGHT_SEC_OFF);
}

void BleAdvSecLight::write_state(light::LightState *state) { (void) state; }

}  // namespace esphome::ble_adv_controller
