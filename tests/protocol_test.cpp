#include "../components/ble_adv_controller/protocol_registry.h"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using ble_adv::protocol::AdvPacket;
using ble_adv::protocol::Command;
using ble_adv::protocol::CommandType;
using ble_adv::protocol::ControllerParams;
using ble_adv::protocol::DecodeResult;
using ble_adv::protocol::Registry;

namespace {

std::string samples_dir() {
  if (const char *env = std::getenv("BLE_ADV_SAMPLES_DIR"))
    return env;
  return "tests/samples";
}

std::string read_file(const std::string &path) {
  std::ifstream in(path);
  std::ostringstream buffer;
  buffer << in.rdbuf();
  return buffer.str();
}

void test_packet_parser() {
  AdvPacket packet;
  assert(packet.from_hex("0x02.01.19.03.03.F0.08 (7)"));
  assert(packet.len == 7);
  assert(packet.ad_flag() == 0x19);
  assert(packet.data_type() == 0x03);
  assert(packet.data_len() == 2);
  assert(!packet.from_hex("0201"));
  assert(!packet.from_hex("xyz"));
}

void test_all_variants_pair_roundtrip() {
  Registry registry;
  assert(registry.encoders().size() == 15);
  std::set<std::string> ids;
  for (const auto &encoder : registry.encoders()) {
    assert(ids.insert(encoder->id()).second);
    ControllerParams params{0x123456, 12, 2, 0x2345};
    Command pair(CommandType::PAIR);
    const auto packets = encoder->encode(pair, params);
    if (packets.empty())
      std::cerr << "encode failed: " << encoder->id() << "\n";
    assert(!packets.empty());
    assert(params.tx_count == 13);
    for (const auto &packet : packets) {
      assert(packet.len > 0 && packet.len <= 31);
      Command decoded;
      ControllerParams decoded_params;
      assert(encoder->decode(packet, decoded, decoded_params));
      assert(decoded.raw_cmd != 0);
      assert(decoded_params.tx_count == 13);
      decoded_params.tx_count--;
      const auto roundtrip = encoder->encode(decoded, decoded_params);
      assert(roundtrip.size() == 1);
      assert(packet.data_len() == roundtrip.front().data_len());
      assert(std::equal(packet.data(), packet.data() + packet.data_len(), roundtrip.front().data()));
    }
  }
}

const std::vector<CommandType> ALL_COMMAND_TYPES = {
    CommandType::PAIR,        CommandType::UNPAIR,       CommandType::LIGHT_ON,
    CommandType::LIGHT_OFF,   CommandType::LIGHT_DIM,    CommandType::LIGHT_CCT,
    CommandType::LIGHT_WCOLOR, CommandType::LIGHT_SEC_ON, CommandType::LIGHT_SEC_OFF,
    CommandType::FAN_ON,      CommandType::FAN_OFF,      CommandType::FAN_SPEED,
    CommandType::FAN_ONOFF_SPEED, CommandType::FAN_DIR, CommandType::FAN_OSC,
};

void test_supported_commands_encode() {
  Registry registry;
  for (const auto &encoder : registry.encoders()) {
    for (const auto type : ALL_COMMAND_TYPES) {
      Command command(type);
      if (type == CommandType::LIGHT_DIM || type == CommandType::LIGHT_CCT)
        command.args[0] = 50;
      if (type == CommandType::LIGHT_WCOLOR)
        command.args = {80, 60, 0, 0};
      if (type == CommandType::FAN_SPEED || type == CommandType::FAN_ONOFF_SPEED)
        command.args = {3, 6, 0, 0};
      if (type == CommandType::FAN_DIR || type == CommandType::FAN_OSC)
        command.args[0] = 1;
      if (!encoder->supports(command))
        continue;
      ControllerParams params{0xABCDEF01, 10, 3, 0x1111};
      const auto packets = encoder->encode(command, params);
      assert(!packets.empty());
      assert(params.tx_count == 11);
      Command decoded;
      ControllerParams decoded_params;
      assert(encoder->decode(packets.front(), decoded, decoded_params));
      decoded_params.tx_count--;
      const auto roundtrip = encoder->encode(decoded, decoded_params);
      assert(roundtrip.size() == 1);
      assert(packets.front().data_len() == roundtrip.front().data_len());
      assert(std::equal(packets.front().data(), packets.front().data() + packets.front().data_len(),
                        roundtrip.front().data()));
    }
  }
}

void test_command_matrix() {
  Registry registry;
  ControllerParams params{0x12345678, 1, 0, 0x3456};
  Command pair(CommandType::PAIR);
  auto packets = registry.encode("fanlamp_pro", "v3", pair, params);
  assert(packets.size() == 1);
  DecodeResult decoded;
  assert(registry.decode(packets.front(), decoded));
  assert(decoded.encoder->encoding() == "fanlamp_pro");
  assert(decoded.encoder->variant() == "v3");
  assert(decoded.command.raw_cmd == 0x28);
  assert(decoded.roundtrip_equal);

  Command fan(CommandType::FAN_ONOFF_SPEED);
  fan.args[0] = 3;
  fan.args[1] = 6;
  assert(registry.supports("fanlamp_pro", "v1", fan));
  assert(registry.supports("zhijia", "v2", Command(CommandType::FAN_SPEED)));
  assert(!registry.supports("zhijia", "v1", Command(CommandType::FAN_SPEED)));
  assert(!registry.supports("zhijia", "v2", Command(CommandType::FAN_DIR)));
}

void test_known_packet() {
  Registry registry;
  AdvPacket packet;
  assert(packet.from_hex(
      "02.01.01.1B.03.F0.08.30.80.B8.F7.E1.27.DB.F4.95.C1.65.7D.A4.9F.67.F6.B6.30.34.8B.53.2B.38.A2"));
  DecodeResult result;
  assert(registry.decode(packet, result));
  assert(result.encoder->encoding() == "lampsmart_pro");
  assert(result.encoder->variant() == "v3");
  assert(result.params.id == 0xB4555A3F);
  assert(result.command.raw_cmd == 0x11);
  assert(result.roundtrip_equal);
}

void test_multi_variant_tx_count() {
  Registry registry;
  ControllerParams params{0x123456, 20, 0, 0x2222};
  const auto packets = registry.encode("fanlamp_pro", "all", Command(CommandType::LIGHT_ON), params);
  assert(packets.size() == 3);
  assert(params.tx_count == 21);
}

void test_forced_id_and_index() {
  Registry registry;
  ControllerParams params_a{0x00ABCDEF, 0, 7, 0x1000};
  ControllerParams params_b{0x00ABCDEF, 0, 7, 0x1000};
  const auto packets_a = registry.encode("fanlamp_pro", "v3", Command(CommandType::LIGHT_ON), params_a);
  const auto packets_b = registry.encode("fanlamp_pro", "v3", Command(CommandType::LIGHT_ON), params_b);
  assert(packets_a.size() == 1 && packets_b.size() == 1);
  assert(packets_a.front() == packets_b.front());

  ControllerParams params_index{0x00ABCDEF, 0, 9, 0x1000};
  const auto packets_index = registry.encode("zhijia", "v2", Command(CommandType::LIGHT_ON), params_index);
  ControllerParams params_other_index{0x00ABCDEF, 0, 1, 0x1000};
  const auto packets_other = registry.encode("zhijia", "v2", Command(CommandType::LIGHT_ON), params_other_index);
  assert(packets_index.size() == 1 && packets_other.size() == 1);
  assert(!(packets_index.front() == packets_other.front()));
}

void test_tx_count_increment() {
  Registry registry;
  ControllerParams params{0x12345678, 120, 0, 0x2222};
  const auto packets = registry.encode("fanlamp_pro", "v3", Command(CommandType::PAIR), params);
  assert(packets.size() == 1);
  assert(params.tx_count == 121);
}

void test_golden_samples() {
  Registry registry;
  const ControllerParams base_params{0x12345678, 5, 2, 0x3456};
  const std::vector<std::string> sample_paths = {
      "fanlamp_pro/v3/pair.hex",
      "fanlamp_pro/v3/light_on.hex",
      "fanlamp_pro/v3/light_off.hex",
      "fanlamp_pro/v3/fan_speed_3.hex",
      "fanlamp_pro/v3/fan_dir_fwd.hex",
      "fanlamp_pro/v3/fan_osc_on.hex",
      "lampsmart_pro/v3/pair.hex",
      "lampsmart_pro/v3/light_on.hex",
      "lampsmart_pro/v3/light_off.hex",
      "lampsmart_pro/v3/fan_speed_3.hex",
      "zhijia/v2/pair.hex",
      "zhijia/v2/light_on.hex",
      "zhijia/v2/light_off.hex",
      "zhijia/v2/light_dim.hex",
      "zhijia/v2/light_cct.hex",
      "zhijia/v2/fan_on.hex",
      "zhijia/v2/fan_speed_3.hex",
  };

  for (const auto &relative : sample_paths) {
    const std::string path = samples_dir() + "/" + relative;
    assert(std::filesystem::exists(path));
    const std::string hex = read_file(path);
    AdvPacket expected;
    assert(expected.from_hex(hex));

    const auto slash = relative.find('/');
    const auto slash2 = relative.find('/', slash + 1);
    const std::string encoding = relative.substr(0, slash);
    const std::string variant = relative.substr(slash + 1, slash2 - slash - 1);
    const std::string name = relative.substr(slash2 + 1, relative.size() - slash2 - 5);

    CommandType type = CommandType::PAIR;
    uint8_t arg0 = 0;
    uint8_t arg1 = 0;
    if (name == "light_on")
      type = CommandType::LIGHT_ON;
    else if (name == "light_off")
      type = CommandType::LIGHT_OFF;
    else if (name == "light_dim") {
      type = CommandType::LIGHT_DIM;
      arg0 = 50;
    } else if (name == "light_cct") {
      type = CommandType::LIGHT_CCT;
      arg0 = 40;
    } else if (name == "fan_on")
      type = CommandType::FAN_ON;
    else if (name == "fan_speed_3") {
      type = encoding == "zhijia" ? CommandType::FAN_SPEED : CommandType::FAN_ONOFF_SPEED;
      arg0 = 3;
      arg1 = 6;
    } else if (name == "fan_dir_fwd") {
      type = CommandType::FAN_DIR;
      arg0 = 1;
    } else if (name == "fan_osc_on") {
      type = CommandType::FAN_OSC;
      arg0 = 1;
    }

    ControllerParams params = base_params;
    Command command(type);
    command.args[0] = arg0;
    command.args[1] = arg1;
    const auto encoded = registry.encode(encoding, variant, command, params);
    assert(encoded.size() == 1);
    assert(encoded.front() == expected);

    DecodeResult decoded;
    assert(registry.decode(expected, decoded));
    assert(decoded.roundtrip_equal);
  }
}

}  // namespace

int main() {
  test_packet_parser();
  test_all_variants_pair_roundtrip();
  test_supported_commands_encode();
  test_command_matrix();
  test_known_packet();
  test_multi_variant_tx_count();
  test_forced_id_and_index();
  test_tx_count_increment();
  test_golden_samples();
  std::cout << "protocol tests passed\n";
  return 0;
}
