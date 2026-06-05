#pragma once

#include "protocol.h"

namespace ble_adv::protocol {

class ZhijiaEncoder : public Encoder {
 public:
  ZhijiaEncoder(std::string encoding, std::string variant);

 protected:
  static uint32_t bytes_to_id(const uint8_t *bytes, size_t len);
  static void id_to_bytes(uint8_t *bytes, uint32_t id, size_t len);
};

class ZhijiaEncoderV0 final : public ZhijiaEncoder {
 public:
  ZhijiaEncoderV0(std::string encoding, std::string variant);

 protected:
  std::vector<Command> translate(const Command &command, const ControllerParams &params) const override;
  void encode_payload(uint8_t *payload, const Command &command, const ControllerParams &params) const override;
  bool decode_payload(uint8_t *payload, Command &command, ControllerParams &params) const override;
};

class ZhijiaEncoderV1 : public ZhijiaEncoder {
 public:
  ZhijiaEncoderV1(std::string encoding, std::string variant);

 protected:
  std::vector<Command> translate(const Command &command, const ControllerParams &params) const override;
  void encode_payload(uint8_t *payload, const Command &command, const ControllerParams &params) const override;
  bool decode_payload(uint8_t *payload, Command &command, ControllerParams &params) const override;
};

class ZhijiaEncoderV2 final : public ZhijiaEncoderV1 {
 public:
  ZhijiaEncoderV2(std::string encoding, std::string variant);

 protected:
  std::vector<Command> translate(const Command &command, const ControllerParams &params) const override;
  void encode_payload(uint8_t *payload, const Command &command, const ControllerParams &params) const override;
  bool decode_payload(uint8_t *payload, Command &command, ControllerParams &params) const override;
};

}  // namespace ble_adv::protocol
