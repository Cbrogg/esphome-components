#include "protocol_fanlamp.h"

#include <algorithm>

namespace ble_adv::protocol {

FanLampEncoder::FanLampEncoder(std::string encoding, std::string variant, std::vector<uint8_t> prefix)
    : Encoder(std::move(encoding), std::move(variant)), prefix_(std::move(prefix)) {}

std::vector<Command> FanLampEncoder::translate(const Command &command, const ControllerParams &) const {
  Command translated(command.type);
  switch (command.type) {
    case CommandType::PAIR:
      translated.raw_cmd = 0x28;
      break;
    case CommandType::UNPAIR:
      translated.raw_cmd = 0x45;
      break;
    case CommandType::LIGHT_ON:
      translated.raw_cmd = 0x10;
      break;
    case CommandType::LIGHT_OFF:
      translated.raw_cmd = 0x11;
      break;
    case CommandType::LIGHT_WCOLOR:
      translated.raw_cmd = 0x21;
      break;
    case CommandType::LIGHT_SEC_ON:
      translated.raw_cmd = 0x12;
      break;
    case CommandType::LIGHT_SEC_OFF:
      translated.raw_cmd = 0x13;
      break;
    case CommandType::FAN_ONOFF_SPEED:
      translated.raw_cmd = 0x31;
      break;
    case CommandType::FAN_DIR:
      translated.raw_cmd = 0x15;
      break;
    case CommandType::FAN_OSC:
      translated.raw_cmd = 0x16;
      break;
    default:
      return {};
  }
  return {translated};
}

FanLampEncoderV1::FanLampEncoderV1(std::string encoding, std::string variant, uint8_t pair_arg3,
                                   bool pair_arg_only_on_pair, bool xor1, uint8_t supplemental_prefix)
    : FanLampEncoder(std::move(encoding), std::move(variant), {0xAA, 0x98, 0x43, 0xAF, 0x0B, 0x46, 0x46, 0x46}),
      pair_arg3_(pair_arg3),
      pair_arg_only_on_pair_(pair_arg_only_on_pair),
      with_second_crc_(supplemental_prefix == 0),
      xor1_(xor1) {
  if (supplemental_prefix != 0)
    this->prefix_.insert(this->prefix_.begin(), supplemental_prefix);
  this->payload_size_ = this->prefix_.size() + 14 + (this->with_second_crc_ ? 2 : 1);
}

std::vector<Command> FanLampEncoderV1::translate(const Command &command, const ControllerParams &params) const {
  auto translated = FanLampEncoder::translate(command, params);
  for (auto &raw : translated) {
    switch (raw.type) {
      case CommandType::PAIR:
        raw.args[0] = params.id & 0xFFU;
        raw.args[1] = (params.id >> 8U) & 0xF0U;
        raw.args[2] = this->pair_arg3_;
        break;
      case CommandType::LIGHT_WCOLOR:
        raw.args[0] = command.args[0];
        raw.args[1] = command.args[1];
        break;
      case CommandType::FAN_ONOFF_SPEED:
        raw.raw_cmd = command.args[1] == 6 ? 0x32 : 0x31;
        raw.args[0] = command.args[0];
        raw.args[1] = command.args[1] == 6 ? 6 : 0;
        break;
      case CommandType::FAN_DIR:
        raw.args[0] = !command.args[0];
        break;
      case CommandType::FAN_OSC:
        raw.args[0] = command.args[0];
        break;
      default:
        break;
    }
  }
  return translated;
}

void FanLampEncoderV1::encode_payload(uint8_t *payload, const Command &command,
                                      const ControllerParams &params) const {
  std::copy(this->prefix_.begin(), this->prefix_.end(), payload);
  uint8_t *data = payload + this->prefix_.size();
  const uint16_t seed = random_seed(params.seed);
  const uint8_t seed8 = seed & 0xFFU;
  const uint16_t group_index =
      static_cast<uint16_t>(params.id & 0xF0FFU) | (static_cast<uint16_t>(params.index & 0x0FU) << 8U);

  data[0] = command.raw_cmd;
  write_le16(data + 1, group_index);
  data[3] = command.args[0];
  data[4] = command.args[1];
  data[5] = this->pair_arg_only_on_pair_ ? command.args[2] : this->pair_arg3_;
  data[6] = params.tx_count;
  data[7] = 0;
  data[8] = this->xor1_ ? static_cast<uint8_t>(seed8 ^ 1U)
                        : static_cast<uint8_t>(seed8 ^ ((params.id >> 16U) & 0xFFU));
  data[9] = this->xor1_ ? static_cast<uint8_t>(seed8 ^ 1U) : seed8;
  write_be16(data + 10, seed);
  write_be16(data + 12, crc16_be(data, 12, static_cast<uint16_t>(~seed)));

  if (this->with_second_crc_) {
    const uint16_t mac_crc = crc16_be(payload + 1, 5, 0xFFFF);
    write_be16(payload + this->payload_size_ - 2, crc16_be(data, 14, mac_crc));
  } else {
    payload[this->payload_size_ - 1] = 0xAA;
  }
  reverse_bits(payload, this->payload_size_);
  whiten_lfsr(payload, this->payload_size_, 0x6F);
}

bool FanLampEncoderV1::decode_payload(uint8_t *payload, Command &command, ControllerParams &params) const {
  whiten_lfsr(payload, this->payload_size_, 0x6F);
  reverse_bits(payload, this->payload_size_);
  if (!std::equal(this->prefix_.begin(), this->prefix_.end(), payload))
    return false;

  uint8_t *data = payload + this->prefix_.size();
  if (data[0] == 0x28 && data[5] != this->pair_arg3_)
    return false;
  if (data[0] != 0x28) {
    const uint8_t expected = this->pair_arg_only_on_pair_ ? 0 : this->pair_arg3_;
    if (data[5] != expected)
      return false;
  }

  const uint16_t seed = read_be16(data + 10);
  const uint8_t seed8 = seed & 0xFFU;
  if (data[9] != (this->xor1_ ? static_cast<uint8_t>(seed8 ^ 1U) : seed8))
    return false;
  if (read_be16(data + 12) != crc16_be(data, 12, static_cast<uint16_t>(~seed)))
    return false;
  if (this->with_second_crc_) {
    const uint16_t mac_crc = crc16_be(payload + 1, 5, 0xFFFF);
    if (read_be16(payload + this->payload_size_ - 2) != crc16_be(data, 14, mac_crc))
      return false;
  } else if (payload[this->payload_size_ - 1] != 0xAA) {
    return false;
  }

  const uint16_t group_index = read_le16(data + 1);
  const uint8_t remote_id = data[8] ^ seed8;
  command.raw_cmd = data[0];
  std::copy(data + 3, data + 6, command.args.begin());
  params.tx_count = data[6];
  params.index = (group_index >> 8U) & 0x0FU;
  params.id = group_index | (static_cast<uint32_t>(remote_id) << 16U);
  params.seed = seed;
  return true;
}

FanLampEncoderV2::FanLampEncoderV2(std::string encoding, std::string variant, std::vector<uint8_t> prefix,
                                   uint16_t device_type, bool with_signature)
    : FanLampEncoder(std::move(encoding), std::move(variant), std::move(prefix)),
      device_type_(device_type),
      with_signature_(with_signature) {
  this->payload_size_ = this->prefix_.size() + 21;
}

std::vector<Command> FanLampEncoderV2::translate(const Command &command, const ControllerParams &params) const {
  auto translated = FanLampEncoder::translate(command, params);
  for (auto &raw : translated) {
    switch (raw.type) {
      case CommandType::LIGHT_WCOLOR:
        raw.args[2] = command.args[0];
        raw.args[3] = command.args[1];
        break;
      case CommandType::FAN_ONOFF_SPEED:
        raw.args[1] = command.args[1] == 6 ? 0x20 : 0;
        raw.args[2] = command.args[0];
        break;
      case CommandType::FAN_DIR:
        raw.args[1] = !command.args[0];
        break;
      case CommandType::FAN_OSC:
        raw.args[1] = command.args[0];
        break;
      default:
        break;
    }
  }
  return translated;
}

uint16_t FanLampEncoderV2::signature(const uint8_t *payload, uint8_t tx_count, uint16_t seed) const {
  uint8_t key[16] = {0, 0, 0, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16};
  key[0] = seed & 0xFFU;
  key[1] = seed >> 8U;
  key[2] = tx_count;
  uint8_t encrypted[16];
  aes128_encrypt_block(key, payload, encrypted);
  const uint16_t result = read_le16(encrypted);
  return result == 0 ? 0xFFFF : result;
}

void FanLampEncoderV2::whiten(uint8_t *data, size_t len, uint8_t seed, uint8_t salt) {
  static constexpr uint8_t BOXES[128] = {
      0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
      0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
      0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
      0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
      0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
      0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
      0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
      0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16,
  };
  for (size_t i = 0; i < len; i++) {
    data[i] ^= BOXES[((seed + i + 9U) & 0x1FU) + (salt & 0x03U) * 0x20U];
    data[i] ^= seed;
  }
}

void FanLampEncoderV2::encode_payload(uint8_t *payload, const Command &command,
                                      const ControllerParams &params) const {
  std::copy(this->prefix_.begin(), this->prefix_.end(), payload);
  uint8_t *data = payload + this->prefix_.size();
  const uint16_t seed = random_seed(params.seed);

  data[0] = params.tx_count;
  write_le16(data + 1, this->device_type_);
  write_le32(data + 3, params.id);
  data[7] = params.index;
  write_le16(data + 8, command.raw_cmd);
  std::copy(command.args.begin(), command.args.end(), data + 10);
  write_le16(data + 14, 0);
  data[16] = 0;
  write_le16(data + 17, seed);
  write_le16(data + 19, 0);
  if (this->with_signature_)
    write_le16(data + 14, this->signature(payload + 1, params.tx_count, seed));

  whiten(payload + 2, this->payload_size_ - 6, seed & 0xFFU);
  write_le16(payload + this->payload_size_ - 2,
             crc16_be(payload, this->payload_size_ - 2, static_cast<uint16_t>(~seed)));
}

bool FanLampEncoderV2::decode_payload(uint8_t *payload, Command &command, ControllerParams &params) const {
  const uint8_t *data_before = payload + this->prefix_.size();
  const uint16_t seed = read_le16(data_before + 17);
  const uint16_t expected_crc = crc16_be(payload, this->payload_size_ - 2, static_cast<uint16_t>(~seed));
  if (read_le16(payload + this->payload_size_ - 2) != expected_crc)
    return false;

  whiten(payload + 2, this->payload_size_ - 6, seed & 0xFFU);
  if (!std::equal(this->prefix_.begin(), this->prefix_.end(), payload))
    return false;
  uint8_t *data = payload + this->prefix_.size();
  if (read_le16(data + 1) != this->device_type_)
    return false;
  const uint16_t encoded_signature = read_le16(data + 14);
  if (this->with_signature_ ? encoded_signature == 0 : encoded_signature != 0)
    return false;
  if (this->with_signature_ && encoded_signature != this->signature(payload + 1, data[0], seed))
    return false;

  params.tx_count = data[0];
  params.id = read_le32(data + 3);
  params.index = data[7];
  command.raw_cmd = read_le16(data + 8) & 0xFFU;
  std::copy(data + 10, data + 14, command.args.begin());
  params.seed = seed;
  return true;
}

}  // namespace ble_adv::protocol
