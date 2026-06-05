#include "protocol_zhijia.h"

#include <algorithm>

namespace ble_adv::protocol {

namespace {

constexpr uint8_t MAC[4] = {0x19, 0x01, 0x10, 0xAA};
constexpr uint8_t UID[3] = {0x19, 0x01, 0x10};

uint8_t scale_255_to_250(uint8_t value) {
  return static_cast<uint8_t>((static_cast<uint16_t>(value) * 250U) / 255U);
}

}  // namespace

ZhijiaEncoder::ZhijiaEncoder(std::string encoding, std::string variant)
    : Encoder(std::move(encoding), std::move(variant)) {}

uint32_t ZhijiaEncoder::bytes_to_id(const uint8_t *bytes, size_t len) {
  uint32_t result = 0;
  for (size_t i = 0; i < len; i++)
    result = (result << 8U) | bytes[i];
  return result;
}

void ZhijiaEncoder::id_to_bytes(uint8_t *bytes, uint32_t id, size_t len) {
  for (size_t i = 0; i < len; i++)
    bytes[len - i - 1] = (id >> (8U * i)) & 0xFFU;
}

ZhijiaEncoderV0::ZhijiaEncoderV0(std::string encoding, std::string variant)
    : ZhijiaEncoder(std::move(encoding), std::move(variant)) {
  this->payload_size_ = 13;
}

std::vector<Command> ZhijiaEncoderV0::translate(const Command &command, const ControllerParams &) const {
  Command raw(command.type);
  switch (command.type) {
    case CommandType::PAIR:
      raw.raw_cmd = 0xB4;
      break;
    case CommandType::UNPAIR:
      raw.raw_cmd = 0xB0;
      break;
    case CommandType::LIGHT_ON:
      raw.raw_cmd = 0xB3;
      break;
    case CommandType::LIGHT_OFF:
      raw.raw_cmd = 0xB2;
      break;
    case CommandType::LIGHT_DIM:
    case CommandType::LIGHT_CCT: {
      raw.raw_cmd = command.type == CommandType::LIGHT_DIM ? 0xB5 : 0xB7;
      const uint16_t value = (static_cast<uint32_t>(command.args[0]) * 1000U) / 255U;
      raw.args[1] = value >> 8U;
      raw.args[2] = value & 0xFFU;
      break;
    }
    case CommandType::LIGHT_SEC_ON:
    case CommandType::LIGHT_SEC_OFF:
      raw.raw_cmd = 0xA6;
      raw.args[0] = command.type == CommandType::LIGHT_SEC_ON ? 1 : 2;
      break;
    default:
      return {};
  }
  return {raw};
}

void ZhijiaEncoderV0::encode_payload(uint8_t *payload, const Command &command,
                                     const ControllerParams &params) const {
  uint8_t uuid[2]{};
  id_to_bytes(uuid, params.id, sizeof(uuid));
  std::reverse_copy(MAC, MAC + 3, payload);
  reverse_bits(payload, 3);
  uint8_t *tx = payload + 3;
  const uint8_t pivot = command.args[2] ^ params.tx_count;
  tx[0] = pivot ^ uuid[0];
  tx[1] = pivot ^ command.args[0];
  tx[2] = pivot ^ params.index;
  tx[3] = pivot ^ command.args[1];
  tx[4] = pivot ^ command.raw_cmd;
  tx[5] = pivot ^ uuid[1];
  tx[6] = command.args[2] ^ uuid[0];
  tx[7] = command.args[0] ^ params.tx_count;
  write_le16(payload + 11, crc16_le(payload, 11, 0));
  whiten_lfsr(payload, this->payload_size_, 0x7F);
  whiten_lfsr(payload, this->payload_size_, 0x37);
}

bool ZhijiaEncoderV0::decode_payload(uint8_t *payload, Command &command, ControllerParams &params) const {
  whiten_lfsr(payload, this->payload_size_, 0x37);
  whiten_lfsr(payload, this->payload_size_, 0x7F);
  if (read_le16(payload + 11) != crc16_le(payload, 11, 0))
    return false;
  uint8_t address[3];
  reverse_bits(payload, 3);
  std::reverse_copy(payload, payload + 3, address);
  if (!std::equal(address, address + 3, MAC))
    return false;

  const uint8_t *tx = payload + 3;
  params.tx_count = tx[0] ^ tx[6];
  command.args[0] = params.tx_count ^ tx[7];
  const uint8_t pivot = tx[1] ^ command.args[0];
  const uint8_t uuid[2] = {static_cast<uint8_t>(pivot ^ tx[0]), static_cast<uint8_t>(pivot ^ tx[5])};
  params.id = bytes_to_id(uuid, sizeof(uuid));
  params.index = pivot ^ tx[2];
  command.raw_cmd = pivot ^ tx[4];
  command.args[1] = pivot ^ tx[3];
  command.args[2] = uuid[0] ^ tx[6];
  return true;
}

ZhijiaEncoderV1::ZhijiaEncoderV1(std::string encoding, std::string variant)
    : ZhijiaEncoder(std::move(encoding), std::move(variant)) {
  this->payload_size_ = 23;
}

std::vector<Command> ZhijiaEncoderV1::translate(const Command &command, const ControllerParams &) const {
  Command raw(command.type);
  switch (command.type) {
    case CommandType::PAIR:
      raw.raw_cmd = 0xA2;
      break;
    case CommandType::UNPAIR:
      raw.raw_cmd = 0xA3;
      break;
    case CommandType::LIGHT_ON:
      raw.raw_cmd = 0xA5;
      break;
    case CommandType::LIGHT_OFF:
      raw.raw_cmd = 0xA6;
      break;
    case CommandType::LIGHT_WCOLOR:
      raw.raw_cmd = 0xA8;
      raw.args[0] = scale_255_to_250(command.args[1]);
      raw.args[1] = scale_255_to_250(command.args[0]);
      break;
    case CommandType::LIGHT_DIM:
      raw.raw_cmd = 0xAD;
      raw.args[0] = scale_255_to_250(command.args[0]);
      break;
    case CommandType::LIGHT_CCT:
      raw.raw_cmd = 0xAE;
      raw.args[0] = scale_255_to_250(command.args[0]);
      break;
    case CommandType::LIGHT_SEC_ON:
      raw.raw_cmd = 0xAF;
      break;
    case CommandType::LIGHT_SEC_OFF:
      raw.raw_cmd = 0xB0;
      break;
    default:
      return {};
  }
  return {raw};
}

void ZhijiaEncoderV1::encode_payload(uint8_t *payload, const Command &command,
                                     const ControllerParams &params) const {
  uint8_t uuid[3]{};
  id_to_bytes(uuid, params.id, sizeof(uuid));
  std::reverse_copy(MAC, MAC + 4, payload);
  reverse_bits(payload, 4);
  uint8_t *tx = payload + 4;
  uint8_t pivot = uuid[1] ^ uuid[2] ^ UID[2];
  pivot ^= static_cast<uint8_t>((pivot & 1U) - 1U);
  uint8_t key = command.args[0] ^ command.args[1] ^ command.args[2] ^ uuid[0] ^ uuid[1] ^ uuid[2] ^
                params.tx_count ^ params.index ^ command.raw_cmd ^ UID[0] ^ UID[1] ^ UID[2];

  tx[0] = pivot ^ command.args[0];
  tx[1] = pivot ^ key;
  tx[2] = pivot ^ uuid[0];
  tx[3] = pivot ^ command.args[1];
  tx[4] = pivot ^ params.tx_count;
  tx[5] = pivot ^ command.args[2];
  tx[6] = pivot ^ params.index;
  tx[7] = pivot ^ UID[0];
  tx[8] = pivot;
  tx[9] = pivot ^ command.raw_cmd;
  tx[10] = pivot ^ UID[1];
  tx[11] = pivot;
  tx[12] = uuid[1] ^ tx[2];
  tx[13] = UID[2] ^ tx[4];
  tx[14] = tx[7];
  tx[15] = uuid[2] ^ tx[9];
  tx[16] = pivot;
  write_le16(payload + 21, crc16_le(payload, 21, 0));
  whiten_lfsr(payload, this->payload_size_, 0x37);
}

bool ZhijiaEncoderV1::decode_payload(uint8_t *payload, Command &command, ControllerParams &params) const {
  whiten_lfsr(payload, this->payload_size_, 0x37);
  if (read_le16(payload + 21) != crc16_le(payload, 21, 0))
    return false;
  uint8_t address[4];
  reverse_bits(payload, 4);
  std::reverse_copy(payload, payload + 4, address);
  if (!std::equal(address, address + 4, MAC))
    return false;
  const uint8_t *tx = payload + 4;
  if (tx[7] != tx[14] || tx[8] != tx[11] || tx[11] != tx[16])
    return false;

  const uint8_t pivot = tx[16];
  const uint8_t uid[3] = {static_cast<uint8_t>(tx[7] ^ pivot), static_cast<uint8_t>(tx[10] ^ pivot),
                          static_cast<uint8_t>(tx[4] ^ tx[13])};
  if (!std::equal(uid, uid + 3, UID))
    return false;
  command.raw_cmd = tx[9] ^ pivot;
  command.args[0] = tx[0] ^ pivot;
  command.args[1] = tx[3] ^ pivot;
  command.args[2] = tx[5] ^ pivot;
  params.tx_count = tx[4] ^ pivot;
  params.index = tx[6] ^ pivot;
  const uint8_t uuid[3] = {static_cast<uint8_t>(tx[2] ^ pivot), static_cast<uint8_t>(tx[2] ^ tx[12]),
                           static_cast<uint8_t>(tx[9] ^ tx[15])};
  params.id = bytes_to_id(uuid, sizeof(uuid));
  uint8_t key = pivot ^ command.args[0] ^ command.args[1] ^ command.args[2] ^ UID[0] ^ UID[1] ^ UID[2] ^
                uuid[0] ^ uuid[1] ^ uuid[2] ^ params.tx_count ^ params.index ^ command.raw_cmd;
  if (key != tx[1])
    return false;
  uint8_t expected_pivot = uuid[1] ^ uuid[2] ^ UID[2];
  expected_pivot ^= static_cast<uint8_t>((expected_pivot & 1U) - 1U);
  return pivot == expected_pivot;
}

ZhijiaEncoderV2::ZhijiaEncoderV2(std::string encoding, std::string variant)
    : ZhijiaEncoderV1(std::move(encoding), std::move(variant)) {
  this->payload_size_ = 24;
}

std::vector<Command> ZhijiaEncoderV2::translate(const Command &command, const ControllerParams &params) const {
  auto translated = ZhijiaEncoderV1::translate(command, params);
  if (!translated.empty())
    return translated;
  Command raw(command.type);
  switch (command.type) {
    case CommandType::FAN_ON:
      raw.raw_cmd = 0xD2;
      break;
    case CommandType::FAN_OFF:
      raw.raw_cmd = 0xD3;
      break;
    case CommandType::FAN_SPEED:
      raw.raw_cmd = 0xDB + (command.args[1] == 3 ? 2 * command.args[0] : command.args[0]);
      break;
    default:
      return {};
  }
  return {raw};
}

void ZhijiaEncoderV2::encode_payload(uint8_t *payload, const Command &command,
                                     const ControllerParams &params) const {
  uint8_t uuid[3]{};
  id_to_bytes(uuid, params.id, sizeof(uuid));
  uint8_t *tx = payload;
  uint8_t key = MAC[0] ^ MAC[1] ^ MAC[2] ^ params.index ^ params.tx_count ^ command.args[0] ^ command.args[1] ^
                command.args[2] ^ uuid[0] ^ uuid[1] ^ uuid[2];
  uint8_t pivot = uuid[0] ^ uuid[1] ^ uuid[2] ^ params.tx_count ^ command.args[1] ^ MAC[0] ^ MAC[2] ^
                  command.raw_cmd;
  pivot = static_cast<uint8_t>(((pivot & 1U) - 1U) ^ pivot);

  tx[0] = command.args[0];
  tx[1] = key;
  tx[2] = uuid[0];
  tx[3] = command.args[1];
  tx[4] = params.tx_count;
  tx[5] = command.args[2];
  tx[6] = params.index;
  tx[7] = MAC[0];
  tx[8] = uuid[0] ^ params.tx_count ^ command.args[1] ^ MAC[0];
  tx[9] = command.raw_cmd;
  tx[10] = MAC[1];
  tx[11] = 0;
  tx[12] = uuid[1] ^ uuid[0];
  tx[13] = MAC[2] ^ params.tx_count;
  tx[14] = uuid[0] ^ params.tx_count ^ command.args[1] ^ command.raw_cmd;
  tx[15] = uuid[2] ^ command.raw_cmd;
  for (size_t i = 0; i < 16; i++)
    tx[i] ^= pivot;
  tx[16] = pivot;
  std::fill(tx + 17, tx + 24, 0);
  whiten_lfsr(payload, this->payload_size_ - 2, 0xD3);
  whiten_lfsr(payload, this->payload_size_, 0x6F);
}

bool ZhijiaEncoderV2::decode_payload(uint8_t *payload, Command &command, ControllerParams &params) const {
  whiten_lfsr(payload, this->payload_size_, 0x6F);
  whiten_lfsr(payload, this->payload_size_ - 2, 0xD3);
  uint8_t *tx = payload;
  const uint8_t pivot = tx[16];
  for (size_t i = 0; i < 16; i++)
    tx[i] ^= pivot;
  params.tx_count = tx[4];
  params.index = tx[6];
  command.raw_cmd = tx[9];
  command.args[0] = tx[0];
  command.args[1] = tx[3];
  command.args[2] = tx[5];
  const uint8_t address[3] = {tx[7], tx[10], static_cast<uint8_t>(tx[13] ^ params.tx_count)};
  if (!std::equal(address, address + 3, MAC))
    return false;
  const uint8_t uuid[3] = {tx[2], static_cast<uint8_t>(tx[12] ^ tx[2]),
                           static_cast<uint8_t>(tx[15] ^ command.raw_cmd)};
  params.id = bytes_to_id(uuid, sizeof(uuid));
  const uint8_t key = address[0] ^ address[1] ^ address[2] ^ params.index ^ params.tx_count ^ command.args[0] ^
                      command.args[1] ^ command.args[2] ^ uuid[0] ^ uuid[1] ^ uuid[2];
  if (key != tx[1])
    return false;
  uint8_t expected_pivot = uuid[0] ^ uuid[1] ^ uuid[2] ^ params.tx_count ^ command.args[1] ^ address[0] ^
                           address[2] ^ command.raw_cmd;
  expected_pivot = static_cast<uint8_t>(((expected_pivot & 1U) - 1U) ^ expected_pivot);
  return pivot == expected_pivot && tx[8] == (uuid[0] ^ params.tx_count ^ command.args[1] ^ address[0]) &&
         tx[11] == 0 && tx[14] == (uuid[0] ^ params.tx_count ^ command.args[1] ^ command.raw_cmd);
}

}  // namespace ble_adv::protocol
