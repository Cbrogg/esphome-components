#pragma once

#include "protocol.h"

#include <cstdint>

namespace esphome::ble_adv_controller::logic {

inline uint8_t normalize_tx_count(uint8_t tx_count) { return tx_count > 120 ? 0 : tx_count; }

inline uint32_t message_duration(bool more_queued, uint32_t min_duration_ms, uint32_t max_duration_ms) {
  return more_queued ? min_duration_ms : max_duration_ms;
}

inline bool should_replace_queued_command(ble_adv::protocol::CommandType type) {
  return type != ble_adv::protocol::CommandType::CUSTOM;
}

}  // namespace esphome::ble_adv_controller::logic
