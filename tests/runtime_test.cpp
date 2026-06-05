#include "../components/ble_adv_controller/controller_logic.h"
#include "../components/ble_adv_controller/protocol_registry.h"

#include <cassert>
#include <iostream>
#include <list>

using ble_adv::protocol::Command;
using ble_adv::protocol::CommandType;
using ble_adv::protocol::Registry;

namespace {

using esphome::ble_adv_controller::logic::message_duration;
using esphome::ble_adv_controller::logic::normalize_tx_count;
using esphome::ble_adv_controller::logic::should_replace_queued_command;

struct QueueItem {
  CommandType type;
};

void simulate_enqueue(std::list<QueueItem> &queue, CommandType type) {
  if (should_replace_queued_command(type))
    queue.remove_if([&](const QueueItem &item) { return item.type == type; });
  queue.push_back({type});
}

void test_tx_count_rollover() {
  assert(normalize_tx_count(120) == 120);
  assert(normalize_tx_count(121) == 0);
  assert(normalize_tx_count(255) == 0);
}

void test_message_duration() {
  assert(message_duration(true, 200, 3000) == 200);
  assert(message_duration(false, 200, 3000) == 3000);
}

void test_queue_dedup() {
  std::list<QueueItem> queue;
  simulate_enqueue(queue, CommandType::LIGHT_ON);
  simulate_enqueue(queue, CommandType::LIGHT_OFF);
  assert(queue.size() == 2);
  simulate_enqueue(queue, CommandType::LIGHT_ON);
  assert(queue.size() == 2);
  assert(queue.front().type == CommandType::LIGHT_OFF);
  assert(queue.back().type == CommandType::LIGHT_ON);

  simulate_enqueue(queue, CommandType::CUSTOM);
  simulate_enqueue(queue, CommandType::CUSTOM);
  assert(queue.size() == 4);
}

void test_protocol_to_scheduler_integration() {
  Registry registry;
  ble_adv::protocol::ControllerParams params{0x12345678, 10, 1, 0x2222};
  const auto packets = registry.encode("fanlamp_pro", "v3", Command(CommandType::PAIR), params);
  assert(packets.size() == 1);
  assert(packets.front().len > 0);
  assert(packets.front().min_duration_ms >= 100);
  assert(params.tx_count == 11);
}

}  // namespace

int main() {
  test_tx_count_rollover();
  test_message_duration();
  test_queue_dedup();
  test_protocol_to_scheduler_integration();
  std::cout << "runtime tests passed\n";
  return 0;
}
