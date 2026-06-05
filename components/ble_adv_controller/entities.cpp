#include "entities.h"

namespace esphome::ble_adv_controller {

void BleAdvProtocolTextSensor::update() {
  if (this->get_parent() == nullptr)
    return;
  this->publish_state(this->get_parent()->get_encoding());
}

void BleAdvLastPacketTextSensor::update() {
  if (this->get_parent() == nullptr || this->get_parent()->get_handler() == nullptr)
    return;
  this->publish_state(this->get_parent()->get_handler()->last_packet_hex());
}

void BleAdvTxCountSensor::update() {
  if (this->get_parent() == nullptr)
    return;
  this->publish_state(this->get_parent()->tx_count());
}

void BleAdvForcedIdSensor::update() {
  if (this->get_parent() == nullptr)
    return;
  this->publish_state(this->get_parent()->get_forced_id());
}

void BleAdvQueueLengthSensor::update() {
  if (this->get_parent() == nullptr)
    return;
  size_t size = this->get_parent()->queue_size();
  if (this->get_parent()->get_handler() != nullptr)
    size += this->get_parent()->get_handler()->scheduled_queue_size();
  this->publish_state(size);
}

void BleAdvDurationNumber::setup() {
  if (this->get_parent() != nullptr)
    this->publish_state(this->get_parent()->get_min_duration());
}

void BleAdvDurationNumber::control(float value) {
  if (this->get_parent() == nullptr)
    return;
  this->get_parent()->set_min_duration(static_cast<uint32_t>(value));
  this->publish_state(value);
}

void BleAdvMaxDurationNumber::setup() {
  if (this->get_parent() != nullptr)
    this->publish_state(this->get_parent()->get_max_duration());
}

void BleAdvMaxDurationNumber::control(float value) {
  if (this->get_parent() == nullptr)
    return;
  this->get_parent()->set_max_duration(static_cast<uint32_t>(value));
  this->publish_state(value);
}

void BleAdvIndexNumber::setup() {
  if (this->get_parent() != nullptr)
    this->publish_state(this->get_parent()->get_index());
}

void BleAdvIndexNumber::control(float value) {
  if (this->get_parent() == nullptr)
    return;
  this->get_parent()->set_index(static_cast<uint8_t>(value));
  this->publish_state(value);
}

void BleAdvForcedIdNumber::setup() {
  if (this->get_parent() != nullptr)
    this->publish_state(this->get_parent()->get_forced_id());
}

void BleAdvForcedIdNumber::control(float value) {
  if (this->get_parent() == nullptr)
    return;
  this->get_parent()->set_forced_id(static_cast<uint32_t>(value));
  this->publish_state(value);
}

void BleAdvEncodingSelect::setup() {
  if (this->get_parent() != nullptr)
    this->publish_state(this->get_parent()->get_encoding());
}

void BleAdvEncodingSelect::control(const std::string &value) {
  if (this->get_parent() == nullptr)
    return;
  this->get_parent()->set_encoding(value);
  this->publish_state(value);
}

void BleAdvVariantSelect::setup() {
  if (this->get_parent() != nullptr)
    this->publish_state(this->get_parent()->get_variant());
}

void BleAdvVariantSelect::control(const std::string &value) {
  if (this->get_parent() == nullptr)
    return;
  this->get_parent()->set_variant(value);
  this->publish_state(value);
}

}  // namespace esphome::ble_adv_controller
