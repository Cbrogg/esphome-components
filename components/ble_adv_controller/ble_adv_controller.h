#pragma once

#include "ble_adv_handler.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <list>
#include <string>
#include <utility>
#include <vector>

namespace esphome::ble_adv_controller {

using protocol::Command;
using protocol::CommandType;

class BleAdvController : public Component {
 public:
  void loop() override;
  void dump_config() override;

  void set_handler(BleAdvHandler *handler) { this->handler_ = handler; }
  void set_encoding(const std::string &encoding) { this->encoding_ = encoding; }
  void set_variant(const std::string &variant) { this->variant_ = variant; }
  void set_min_duration(uint32_t duration) { this->min_duration_ = duration; }
  void set_max_duration(uint32_t duration) { this->max_duration_ = duration; }
  void set_sequence_duration(uint32_t duration) { this->sequence_duration_ = duration; }
  void set_forced_id(uint32_t forced_id) { this->params_.id = forced_id; }
  void set_forced_id(const std::string &id) { this->params_.id = fnv1_hash(id); }
  void set_index(uint8_t index) { this->params_.index = index; }
  void set_reversed(bool reversed) { this->reversed_ = reversed; }

  bool is_reversed() const { return this->reversed_; }
  const std::string &get_encoding() const { return this->encoding_; }
  const std::string &get_variant() const { return this->variant_; }
  uint32_t get_forced_id() const { return this->params_.id; }
  uint8_t get_index() const { return this->params_.index; }
  uint32_t get_min_duration() const { return this->min_duration_; }
  uint32_t get_max_duration() const { return this->max_duration_; }
  BleAdvHandler *get_handler() const { return this->handler_; }
  bool supports(CommandType type) const;
  bool enqueue(Command command);
  bool inject_raw(const std::string &raw);
  bool decode_raw(const std::string &raw);
  size_t queue_size() const { return this->queue_.size() + (this->active_message_id_ != 0 ? 1 : 0); }
  uint8_t tx_count() const { return this->params_.tx_count; }

 protected:
  struct QueueItem {
    CommandType type;
    std::vector<protocol::AdvPacket> packets;
  };

  BleAdvHandler *handler_{nullptr};
  std::string encoding_;
  std::string variant_;
  protocol::ControllerParams params_;
  uint32_t min_duration_{200};
  uint32_t max_duration_{3000};
  uint32_t sequence_duration_{100};
  bool reversed_{false};
  std::list<QueueItem> queue_;
  uint16_t active_message_id_{0};
  uint32_t active_started_at_{0};
};

class BleAdvEntity : public Component, public Parented<BleAdvController> {
 protected:
  bool command(CommandType type, uint8_t arg0 = 0, uint8_t arg1 = 0);
  bool command(CommandType type, const std::vector<uint8_t> &args);
};

template<typename... Ts> class PairAction : public Action<Ts...>, public Parented<BleAdvController> {
 public:
  void play(const Ts &...x) override { this->get_parent()->enqueue(Command(CommandType::PAIR)); }
};

template<typename... Ts> class UnpairAction : public Action<Ts...>, public Parented<BleAdvController> {
 public:
  void play(const Ts &...x) override { this->get_parent()->enqueue(Command(CommandType::UNPAIR)); }
};

template<typename... Ts> class CommandAction : public Action<Ts...>, public Parented<BleAdvController> {
 public:
  TEMPLATABLE_VALUE(uint8_t, command)
  TEMPLATABLE_VALUE(uint8_t, arg0)
  TEMPLATABLE_VALUE(uint8_t, arg1)
  TEMPLATABLE_VALUE(uint8_t, arg2)
  TEMPLATABLE_VALUE(uint8_t, arg3)

  void play(const Ts &...x) override {
    Command command(CommandType::CUSTOM);
    command.raw_cmd = this->command_.value(x...);
    command.args = {this->arg0_.value(x...), this->arg1_.value(x...), this->arg2_.value(x...), this->arg3_.value(x...)};
    this->get_parent()->enqueue(command);
  }
};

template<typename... Ts> class RawInjectAction : public Action<Ts...>, public Parented<BleAdvController> {
 public:
  TEMPLATABLE_VALUE(std::string, raw)
  void play(const Ts &...x) override { this->get_parent()->inject_raw(this->raw_.value(x...)); }
};

template<typename... Ts> class RawDecodeAction : public Action<Ts...>, public Parented<BleAdvController> {
 public:
  TEMPLATABLE_VALUE(std::string, raw)
  void play(const Ts &...x) override { this->get_parent()->decode_raw(this->raw_.value(x...)); }
};

}  // namespace esphome::ble_adv_controller
