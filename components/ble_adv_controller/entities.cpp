#include "entities.h"

#include <cstdio>
#include <string>

namespace esphome::ble_adv_controller {

namespace {

std::string shorten_hex(const std::string &hex, size_t max_visible = 20) {
  if (hex.size() <= max_visible)
    return hex;
  return hex.substr(0, max_visible) + "…";
}

}  // namespace

void BleAdvProtocolTextSensor::update() {
  if (this->get_parent() == nullptr)
    return;
  this->publish_state(this->get_parent()->get_encoding());
}

void BleAdvVariantTextSensor::update() {
  if (this->get_parent() == nullptr)
    return;
  this->publish_state(this->get_parent()->get_variant());
}

void BleAdvLastPacketTextSensor::update() {
  if (this->get_parent() == nullptr || this->get_parent()->get_handler() == nullptr)
    return;
  this->publish_state(shorten_hex(this->get_parent()->get_handler()->last_packet_hex()));
}

void BleAdvForcedIdTextSensor::update() {
  if (this->get_parent() == nullptr)
    return;
  char buffer[16];
  std::snprintf(buffer, sizeof(buffer), "0x%08" PRIX32, this->get_parent()->get_forced_id());
  this->publish_state(buffer);
}

void BleAdvTxCountSensor::update() {
  if (this->get_parent() == nullptr)
    return;
  this->publish_state(this->get_parent()->tx_count());
}

void BleAdvQueueLengthSensor::update() {
  if (this->get_parent() == nullptr)
    return;
  size_t size = this->get_parent()->queue_size();
  if (this->get_parent()->get_handler() != nullptr)
    size += this->get_parent()->get_handler()->scheduled_queue_size();
  this->publish_state(size);
}

void BleAdvDurationNumber::control(float value) {
  if (this->get_parent() == nullptr)
    return;
  this->get_parent()->set_min_duration(static_cast<uint32_t>(value));
  this->publish_state(value);
}

void BleAdvMaxDurationNumber::control(float value) {
  if (this->get_parent() == nullptr)
    return;
  this->get_parent()->set_max_duration(static_cast<uint32_t>(value));
  this->publish_state(value);
}

void BleAdvIndexNumber::control(float value) {
  if (this->get_parent() == nullptr)
    return;
  this->get_parent()->set_index(static_cast<uint8_t>(value));
  this->publish_state(value);
}

void BleAdvForcedIdNumber::control(float value) {
  if (this->get_parent() == nullptr)
    return;
  this->get_parent()->set_forced_id(static_cast<uint32_t>(value));
  this->publish_state(value);
}

void BleAdvEncodingSelect::control(const std::string &value) {
  if (this->get_parent() == nullptr)
    return;
  this->get_parent()->set_encoding(value);
  this->publish_state(value);
}

void BleAdvVariantSelect::control(const std::string &value) {
  if (this->get_parent() == nullptr)
    return;
  this->get_parent()->set_variant(value);
  this->publish_state(value);
}

}  // namespace esphome::ble_adv_controller
