#include "ble_adv_controller.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "esphome/core/application.h"
#include "esphome/core/preferences.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace bleadvcontroller {

static const char *const TAG = "ble_adv_controller";

void BleAdvSelect::control(const std::string &value) {
  this->publish_state(value);
  uint32_t hash_value = fnv1_hash(value);
  this->rtc_.save(&hash_value);
}

void BleAdvSelect::sub_init() { 
  // App.register_select wurde entfernt, da die Registrierung nun automatisch erfolgt.
  this->rtc_ = global_preferences->make_preference<uint32_t>(this->get_object_id_hash());
  uint32_t restored;
  if (this->rtc_.load(&restored)) {
    const auto &options = this->traits.get_options();
    for (auto &opt : options) {
      if(fnv1_hash(opt) == restored) {
        this->publish_state(opt);
        return;
      }
    }
  }
}

void BleAdvNumber::control(float value) {
  this->publish_state(value);
  this->rtc_.save(&value);
}

void BleAdvNumber::sub_init() {
  // App.register_number wurde entfernt.
  this->rtc_ = global_preferences->make_preference<float>(this->get_object_id_hash());
  float restored;
  if (this->rtc_.load(&restored)) {
    this->publish_state(restored);
  }
}

void BleAdvController::set_encoding_and_variant(const std::string &encoding, const std::string &variant) {
  auto ids = this->handler_->get_ids(encoding);
  FixedVector<const char *> options;
  options.init(ids.size());
  for (const auto &id : ids) {
    options.push_back(id.c_str());
  }
  this->select_encoding_.traits.set_options(std::move(options));

  this->cur_encoder_ = this->handler_->get_encoder(encoding, variant);
  this->select_encoding_.publish_state(this->cur_encoder_->get_id());
  this->select_encoding_.add_on_state_callback([this](size_t index) {
    const auto &options = this->select_encoding_.traits.get_options();
    if (index < options.size()) {
      this->refresh_encoder(options[index], index);
    }
  });
}

void BleAdvController::refresh_encoder(std::string id, size_t index) {
  this->cur_encoder_ = this->handler_->get_encoder(id);
}

void BleAdvController::set_min_tx_duration(int tx_duration, int min, int max, int step) {
  this->number_duration_.traits.set_min_value(min);
  this->number_duration_.traits.set_max_value(max);
  this->number_duration_.traits.set_step(step);
  this->number_duration_.publish_state(tx_duration);
}

void BleAdvController::setup() {
#ifdef USE_API
  // Wir nutzen get_name() statt get_object_id()
  register_service(&BleAdvController::on_pair, "pair_" + this->get_name());
  register_service(&BleAdvController::on_unpair, "unpair_" + this->get_name());
  register_service(&BleAdvController::on_cmd, "cmd_" + this->get_name(), {"cmd", "arg0", "arg1", "arg2", "arg3"});
  register_service(&BleAdvController::on_raw_inject, "inject_raw_" + this->get_name(), {"raw"});
#endif
  if (this->is_show_config()) {
    this->select_encoding_.sub_init();
    this->number_duration_.sub_init();
  }
}

void BleAdvController::dump_config() {
  ESP_LOGCONFIG(TAG, "BleAdvController '%s'", this->get_name().c_str());
  ESP_LOGCONFIG(TAG, "  Hash ID '%lX'", this->params_.id_);
  ESP_LOGCONFIG(TAG, "  Index '%d'", this->params_.index_);
  ESP_LOGCONFIG(TAG, "  Transmission Min Duration: %d ms", (int)this->get_min_tx_duration());
  ESP_LOGCONFIG(TAG, "  Transmission Max Duration: %d ms", (int)this->max_tx_duration_);
  ESP_LOGCONFIG(TAG, "  Transmission Sequencing Duration: %d ms", (int)this->seq_duration_);
  ESP_LOGCONFIG(TAG, "  Configuration visible: %s", this->show_config_ ? "YES" : "NO");
}

#ifdef USE_API
void BleAdvController::on_pair() { 
  Command cmd(CommandType::PAIR);
  this->enqueue(cmd);
}

void BleAdvController::on_unpair() {
  Command cmd(CommandType::UNPAIR);
  this->enqueue(cmd);
}

void BleAdvController::on_cmd(float cmd_type, float arg0, float arg1, float arg2, float arg3) {
  Command cmd(CommandType::CUSTOM);
  cmd.cmd_ = (uint8_t)cmd_type;
  cmd.args_[0] = (uint8_t)arg0;
  cmd.args_[1] = (uint8_t)arg1;
  cmd.args_[2] = (uint8_t)arg2;
  cmd.args_[3] = (uint8_t)arg3;
  this->enqueue(cmd);
}

void BleAdvController::on_raw_inject(std::string raw) {
  this->commands_.emplace_back(CommandType::CUSTOM);
  this->commands_.back().params_.emplace_back();
  this->commands_.back().params_.back().from_hex_string(raw);
}
#endif

bool BleAdvController::enqueue(Command &cmd) {
  if (this->cur_encoder_ == nullptr || !this->cur_encoder_->is_supported(cmd)) {
    ESP_LOGW(TAG, "Unsupported command or no encoder. Aborted.");
    return false;
  }

  if (this->params_.tx_count_ > 120) {
    this->params_.tx_count_ = 0;
  }

  if (cmd.main_cmd_ != CommandType::CUSTOM) {
    this->commands_.remove_if([&](QueueItem& q){ return q.cmd_type_ == cmd.main_cmd_; });
  }

  this->commands_.emplace_back(cmd.main_cmd_);
  this->cur_encoder_->encode(this->commands_.back().params_, cmd, this->params_);
  
  bool use_seq_duration = (this->seq_duration_ > 0) && (this->seq_duration_ < this->get_min_tx_duration());
  for (auto & param : this->commands_.back().params_) {
    param.duration_ = use_seq_duration ? (uint32_t)this->seq_duration_: (uint32_t)this->get_min_tx_duration();
  }
  
  return true;
}

void BleAdvController::loop() {
  uint32_t now = millis();
  if(this->adv_start_time_ == 0) {
    if(!this->commands_.empty()) {
      QueueItem & item = this->commands_.front();
      this->adv_id_ = this->handler_->add_to_advertiser(item.params_);
      this->adv_start_time_ = now;
      this->commands_.pop_front();
    }
  }
  else {
    uint32_t duration = this->commands_.empty() ? (uint32_t)this->max_tx_duration_ : (uint32_t)this->number_duration_.state;
    if (now > this->adv_start_time_ + duration) {
      this->adv_start_time_ = 0;
      this->handler_->remove_from_advertiser(this->adv_id_);
    }
  }
}

void BleAdvEntity::dump_config_base(const char * tag) {
  ESP_LOGCONFIG(tag, "  Controller '%s'", this->get_parent()->get_name().c_str());
}

void BleAdvEntity::command(CommandType cmd_type, const std::vector<uint8_t> &args) {
  Command cmd(cmd_type);
  size_t n = std::min(args.size(), (size_t)4);
  for(size_t i=0; i<n; i++) cmd.args_[i] = args[i];
  this->get_parent()->enqueue(cmd);
}

void BleAdvEntity::command(CommandType cmd, uint8_t value1, uint8_t value2) {
  this->command(cmd, {value1, value2});
}

} // namespace bleadvcontroller
} // namespace esphome
