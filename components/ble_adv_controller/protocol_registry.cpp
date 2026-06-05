#include "protocol_registry.h"

#include <algorithm>

namespace ble_adv::protocol {

namespace {

const std::vector<VariantDefinition> DEFINITIONS = {
    {"fanlamp_pro", "v1", 0, 0xFFFFFF},   {"fanlamp_pro", "v2", 0, 0xFFFFFFFF},
    {"fanlamp_pro", "v3", 0, 0xFFFFFFFF}, {"lampsmart_pro", "v1", 0, 0xFFFFFF},
    {"lampsmart_pro", "v2", 0, 0xFFFFFFFF},
    {"lampsmart_pro", "v3", 0, 0xFFFFFFFF},
    {"zhijia", "v0", 0xC630B8, 0xFFFF},   {"zhijia", "v1", 0xC630B8, 0xFFFFFF},
    {"zhijia", "v2", 0xC630B8, 0xFFFFFF}, {"remote", "v1", 0, 0xFFFFFF},
    {"remote", "v3", 0, 0xFFFFFFFF},      {"other", "v1a", 0, 0xFFFFFF},
    {"other", "v1b", 0, 0xFFFFFF},        {"other", "v2", 0, 0xFFFFFFFF},
    {"other", "v3", 0, 0xFFFFFFFF},
};

template<typename T>
T *configure(T *encoder, uint8_t ad_flag, uint8_t data_type, std::vector<uint8_t> header) {
  encoder->set_ble_params(ad_flag, data_type);
  encoder->set_header(std::move(header));
  return encoder;
}

bool packet_data_equal(const AdvPacket &left, const AdvPacket &right) {
  return left.data_len() == right.data_len() && left.data() != nullptr && right.data() != nullptr &&
         std::equal(left.data(), left.data() + left.data_len(), right.data());
}

}  // namespace

Registry::Registry() {
  configure(this->add<FanLampEncoderV1>("fanlamp_pro", "v1", 0x83, false), 0x19, 0x03, {0x77, 0xF8});
  configure(this->add<FanLampEncoderV2>("fanlamp_pro", "v2", std::vector<uint8_t>{0x10, 0x80, 0x00}, 0x0400,
                                        false),
            0x19, 0x03, {0xF0, 0x08});
  configure(this->add<FanLampEncoderV2>("fanlamp_pro", "v3", std::vector<uint8_t>{0x20, 0x80, 0x00}, 0x0400,
                                        true),
            0x19, 0x03, {0xF0, 0x08});

  configure(this->add<FanLampEncoderV1>("lampsmart_pro", "v1", 0x81), 0x19, 0x03, {0x77, 0xF8});
  configure(this->add<FanLampEncoderV2>("lampsmart_pro", "v2", std::vector<uint8_t>{0x10, 0x80, 0x00}, 0x0100,
                                        false),
            0x19, 0x03, {0xF0, 0x08});
  configure(this->add<FanLampEncoderV2>("lampsmart_pro", "v3", std::vector<uint8_t>{0x30, 0x80, 0x00}, 0x0100,
                                        true),
            0x19, 0x03, {0xF0, 0x08});

  configure(this->add<ZhijiaEncoderV0>("zhijia", "v0"), 0x1A, 0xFF, {0xF9, 0x08, 0x49});
  configure(this->add<ZhijiaEncoderV1>("zhijia", "v1"), 0x1A, 0xFF, {0xF9, 0x08, 0x49});
  configure(this->add<ZhijiaEncoderV2>("zhijia", "v2"), 0x1A, 0xFF, {0x22, 0x9D});

  configure(this->add<FanLampEncoderV1>("remote", "v1", 0x83, false, true), 0x00, 0xFF,
            {0x56, 0x55, 0x18, 0x87, 0x52});
  configure(this->add<FanLampEncoderV2>("remote", "v3", std::vector<uint8_t>{0x10, 0x00, 0x56}, 0x0400, true),
            0x02, 0x16, {0xF0, 0x08});

  configure(this->add<FanLampEncoderV1>("other", "v1a", 0x81, true, true), 0x02, 0x03, {0x77, 0xF8});
  configure(this->add<FanLampEncoderV1>("other", "v1b", 0x81, true, true, 0x55), 0x02, 0x16, {0xF9, 0x08});
  configure(this->add<FanLampEncoderV2>("other", "v2", std::vector<uint8_t>{0x10, 0x80, 0x00}, 0x0100, false),
            0x19, 0x16, {0xF0, 0x08});
  configure(this->add<FanLampEncoderV2>("other", "v3", std::vector<uint8_t>{0x10, 0x80, 0x00}, 0x0100, true),
            0x19, 0x16, {0xF0, 0x08});
}

Encoder *Registry::find(const std::string &encoding, const std::string &variant) const {
  const auto match = std::find_if(this->encoders_.begin(), this->encoders_.end(), [&](const auto &encoder) {
    return encoder->encoding() == encoding && encoder->variant() == variant;
  });
  return match == this->encoders_.end() ? nullptr : match->get();
}

std::vector<Encoder *> Registry::variants(const std::string &encoding) const {
  std::vector<Encoder *> result;
  for (const auto &encoder : this->encoders_) {
    if (encoder->encoding() == encoding)
      result.push_back(encoder.get());
  }
  return result;
}

std::vector<std::string> Registry::variant_names(const std::string &encoding) const {
  std::vector<std::string> result;
  for (const auto *encoder : this->variants(encoding))
    result.push_back(encoder->variant());
  return result;
}

std::vector<AdvPacket> Registry::encode(const std::string &encoding, const std::string &variant,
                                        const Command &command, ControllerParams &params) const {
  if (variant != "all") {
    const auto *encoder = this->find(encoding, variant);
    return encoder == nullptr ? std::vector<AdvPacket>{} : encoder->encode(command, params);
  }
  MultiEncoder multi(encoding, this->variants(encoding));
  return multi.encode_all(command, params);
}

bool Registry::supports(const std::string &encoding, const std::string &variant, const Command &command) const {
  if (variant != "all") {
    const auto *encoder = this->find(encoding, variant);
    return encoder != nullptr && encoder->supports(command);
  }
  MultiEncoder multi(encoding, this->variants(encoding));
  return multi.supports_any(command);
}

bool Registry::decode(const AdvPacket &packet, DecodeResult &result, bool ignore_ble_params) const {
  for (const auto &candidate : this->encoders_) {
    if (!ignore_ble_params && !candidate->matches_ble_params(packet))
      continue;
    Command command;
    ControllerParams params;
    if (!candidate->decode(packet, command, params))
      continue;
    ControllerParams reencode_params = params;
    reencode_params.tx_count--;
    const auto packets = candidate->encode(command, reencode_params);
    result.encoder = candidate.get();
    result.command = command;
    result.params = params;
    if (!packets.empty()) {
      result.reencoded = packets.front();
      result.roundtrip_equal = packet_data_equal(packet, result.reencoded);
    }
    return true;
  }
  return false;
}

const std::vector<VariantDefinition> &variant_definitions() { return DEFINITIONS; }

const VariantDefinition *find_definition(const std::string &encoding, const std::string &variant) {
  const auto match = std::find_if(DEFINITIONS.begin(), DEFINITIONS.end(), [&](const auto &definition) {
    return definition.encoding == encoding && definition.variant == variant;
  });
  return match == DEFINITIONS.end() ? nullptr : &*match;
}

}  // namespace ble_adv::protocol
