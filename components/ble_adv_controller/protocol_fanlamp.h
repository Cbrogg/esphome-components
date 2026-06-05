#pragma once

#include "protocol.h"

namespace ble_adv::protocol {

class FanLampEncoder : public Encoder {
 public:
  FanLampEncoder(std::string encoding, std::string variant, std::vector<uint8_t> prefix);

 protected:
  std::vector<Command> translate(const Command &command, const ControllerParams &params) const override;

  std::vector<uint8_t> prefix_;
};

class FanLampEncoderV1 final : public FanLampEncoder {
 public:
  FanLampEncoderV1(std::string encoding, std::string variant, uint8_t pair_arg3,
                   bool pair_arg_only_on_pair = true, bool xor1 = false, uint8_t supplemental_prefix = 0);

 protected:
  std::vector<Command> translate(const Command &command, const ControllerParams &params) const override;
  void encode_payload(uint8_t *payload, const Command &command, const ControllerParams &params) const override;
  bool decode_payload(uint8_t *payload, Command &command, ControllerParams &params) const override;

  uint8_t pair_arg3_;
  bool pair_arg_only_on_pair_;
  bool with_second_crc_;
  bool xor1_;
};

class FanLampEncoderV2 final : public FanLampEncoder {
 public:
  FanLampEncoderV2(std::string encoding, std::string variant, std::vector<uint8_t> prefix, uint16_t device_type,
                   bool with_signature);

 protected:
  std::vector<Command> translate(const Command &command, const ControllerParams &params) const override;
  void encode_payload(uint8_t *payload, const Command &command, const ControllerParams &params) const override;
  bool decode_payload(uint8_t *payload, Command &command, ControllerParams &params) const override;

  uint16_t signature(const uint8_t *payload, uint8_t tx_count, uint16_t seed) const;
  static void whiten(uint8_t *data, size_t len, uint8_t seed, uint8_t salt = 0);

  uint16_t device_type_;
  bool with_signature_;
};

}  // namespace ble_adv::protocol
