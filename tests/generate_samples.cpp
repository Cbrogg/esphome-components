#include "../components/ble_adv_controller/protocol_registry.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using ble_adv::protocol::AdvPacket;
using ble_adv::protocol::Command;
using ble_adv::protocol::CommandType;
using ble_adv::protocol::ControllerParams;
using ble_adv::protocol::Registry;

namespace {

struct SampleSpec {
  const char *encoding;
  const char *variant;
  const char *name;
  CommandType type;
  uint8_t arg0;
  uint8_t arg1;
};

const ControllerParams BASE_PARAMS{0x12345678, 5, 2, 0x3456};

void write_sample(const std::string &path, const AdvPacket &packet) {
  std::filesystem::create_directories(std::filesystem::path(path).parent_path());
  std::ofstream out(path);
  out << packet.to_hex();
}

Command make_command(const SampleSpec &spec) {
  Command command(spec.type);
  command.args[0] = spec.arg0;
  command.args[1] = spec.arg1;
  return command;
}

}  // namespace

int main() {
  Registry registry;
  const std::vector<SampleSpec> specs = {
      {"fanlamp_pro", "v3", "pair", CommandType::PAIR, 0, 0},
      {"fanlamp_pro", "v3", "light_on", CommandType::LIGHT_ON, 0, 0},
      {"fanlamp_pro", "v3", "light_off", CommandType::LIGHT_OFF, 0, 0},
      {"fanlamp_pro", "v3", "fan_speed_3", CommandType::FAN_ONOFF_SPEED, 3, 6},
      {"fanlamp_pro", "v3", "fan_dir_fwd", CommandType::FAN_DIR, 1, 0},
      {"fanlamp_pro", "v3", "fan_osc_on", CommandType::FAN_OSC, 1, 0},
      {"lampsmart_pro", "v3", "pair", CommandType::PAIR, 0, 0},
      {"lampsmart_pro", "v3", "light_on", CommandType::LIGHT_ON, 0, 0},
      {"lampsmart_pro", "v3", "light_off", CommandType::LIGHT_OFF, 0, 0},
      {"lampsmart_pro", "v3", "fan_speed_3", CommandType::FAN_ONOFF_SPEED, 3, 6},
      {"zhijia", "v2", "pair", CommandType::PAIR, 0, 0},
      {"zhijia", "v2", "light_on", CommandType::LIGHT_ON, 0, 0},
      {"zhijia", "v2", "light_off", CommandType::LIGHT_OFF, 0, 0},
      {"zhijia", "v2", "light_dim", CommandType::LIGHT_DIM, 50, 0},
      {"zhijia", "v2", "light_cct", CommandType::LIGHT_CCT, 40, 0},
      {"zhijia", "v2", "fan_on", CommandType::FAN_ON, 0, 0},
      {"zhijia", "v2", "fan_speed_3", CommandType::FAN_SPEED, 3, 6},
  };

  for (const auto &spec : specs) {
    ControllerParams params = BASE_PARAMS;
    Command command = make_command(spec);
    if (!registry.supports(spec.encoding, spec.variant, command)) {
      std::fprintf(stderr, "unsupported: %s/%s %s\n", spec.encoding, spec.variant, spec.name);
      return 1;
    }
    auto packets = registry.encode(spec.encoding, spec.variant, command, params);
    if (packets.size() != 1) {
      std::fprintf(stderr, "encode failed: %s/%s %s\n", spec.encoding, spec.variant, spec.name);
      return 1;
    }
    const std::string path = std::string("tests/samples/") + spec.encoding + "/" + spec.variant + "/" + spec.name +
                             ".hex";
    write_sample(path, packets.front());
    std::printf("wrote %s\n", path.c_str());
  }
  return 0;
}
