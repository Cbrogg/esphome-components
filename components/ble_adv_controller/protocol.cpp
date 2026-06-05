#include "protocol.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>

namespace ble_adv::protocol {

namespace {

constexpr uint8_t AD_TYPE_FLAGS = 0x01;
constexpr uint8_t AES_SBOX[256] = {
    0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
    0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
    0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
    0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
    0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, 0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
    0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
    0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
    0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
    0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
    0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
    0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
    0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
    0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
    0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E, 0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
    0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
    0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16,
};
constexpr uint8_t AES_RCON[10] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1B, 0x36};

uint8_t aes_xtime(uint8_t value) { return static_cast<uint8_t>((value << 1U) ^ ((value & 0x80U) ? 0x1BU : 0U)); }

void aes_add_round_key(uint8_t state[16], const uint8_t *round_key) {
  for (size_t i = 0; i < 16; i++)
    state[i] ^= round_key[i];
}

void aes_sub_bytes(uint8_t state[16]) {
  for (auto i = 0U; i < 16; i++)
    state[i] = AES_SBOX[state[i]];
}

void aes_shift_rows(uint8_t state[16]) {
  uint8_t copy[16];
  std::copy(state, state + 16, copy);
  for (size_t row = 0; row < 4; row++) {
    for (size_t column = 0; column < 4; column++)
      state[row + 4 * column] = copy[row + 4 * ((column + row) % 4)];
  }
}

void aes_mix_columns(uint8_t state[16]) {
  for (size_t column = 0; column < 4; column++) {
    uint8_t *values = state + column * 4;
    const uint8_t total = values[0] ^ values[1] ^ values[2] ^ values[3];
    const uint8_t first = values[0];
    values[0] ^= total ^ aes_xtime(values[0] ^ values[1]);
    values[1] ^= total ^ aes_xtime(values[1] ^ values[2]);
    values[2] ^= total ^ aes_xtime(values[2] ^ values[3]);
    values[3] ^= total ^ aes_xtime(values[3] ^ first);
  }
}

void aes_expand_key(const uint8_t key[16], uint8_t expanded[176]) {
  std::copy(key, key + 16, expanded);
  size_t generated = 16;
  size_t rcon_index = 0;
  uint8_t temp[4];
  while (generated < 176) {
    std::copy(expanded + generated - 4, expanded + generated, temp);
    if ((generated % 16) == 0) {
      const uint8_t first = temp[0];
      temp[0] = AES_SBOX[temp[1]] ^ AES_RCON[rcon_index++];
      temp[1] = AES_SBOX[temp[2]];
      temp[2] = AES_SBOX[temp[3]];
      temp[3] = AES_SBOX[first];
    }
    for (size_t i = 0; i < 4; i++) {
      expanded[generated] = expanded[generated - 16] ^ temp[i];
      generated++;
    }
  }
}

int hex_value(char value) {
  if (value >= '0' && value <= '9')
    return value - '0';
  value = static_cast<char>(std::tolower(static_cast<unsigned char>(value)));
  if (value >= 'a' && value <= 'f')
    return value - 'a' + 10;
  return -1;
}

}  // namespace

uint16_t read_le16(const uint8_t *data) { return static_cast<uint16_t>(data[0] | (data[1] << 8U)); }
uint16_t read_be16(const uint8_t *data) { return static_cast<uint16_t>((data[0] << 8U) | data[1]); }
uint32_t read_le32(const uint8_t *data) {
  return static_cast<uint32_t>(data[0]) | (static_cast<uint32_t>(data[1]) << 8U) |
         (static_cast<uint32_t>(data[2]) << 16U) | (static_cast<uint32_t>(data[3]) << 24U);
}
void write_le16(uint8_t *data, uint16_t value) {
  data[0] = value & 0xFFU;
  data[1] = value >> 8U;
}
void write_be16(uint8_t *data, uint16_t value) {
  data[0] = value >> 8U;
  data[1] = value & 0xFFU;
}
void write_le32(uint8_t *data, uint32_t value) {
  data[0] = value & 0xFFU;
  data[1] = (value >> 8U) & 0xFFU;
  data[2] = (value >> 16U) & 0xFFU;
  data[3] = (value >> 24U) & 0xFFU;
}

bool AdvPacket::from_raw(const uint8_t *data, size_t data_len) {
  this->bytes.fill(0);
  this->len = 0;
  this->ad_flag_index = MAX_PACKET_LEN;
  this->data_index = MAX_PACKET_LEN;
  if (data == nullptr || data_len == 0 || data_len > MAX_PACKET_LEN)
    return false;
  std::copy(data, data + data_len, this->bytes.begin());
  this->len = data_len;

  size_t offset = 0;
  while (offset < this->len) {
    const size_t section_len = this->bytes[offset];
    if (section_len == 0)
      break;
    if (section_len < 1 || offset + section_len + 1 > this->len) {
      this->len = 0;
      return false;
    }
    const uint8_t type = this->bytes[offset + 1];
    if (type == AD_TYPE_FLAGS)
      this->ad_flag_index = offset;
    if (type == 0xFF || type == 0x03 || type == 0x16)
      this->data_index = offset;
    offset += section_len + 1;
  }
  return true;
}

bool AdvPacket::from_hex(const std::string &raw) {
  size_t input_offset = 0;
  while (input_offset < raw.size() && std::isspace(static_cast<unsigned char>(raw[input_offset])))
    input_offset++;
  if (input_offset + 1 < raw.size() && raw[input_offset] == '0' &&
      (raw[input_offset + 1] == 'x' || raw[input_offset + 1] == 'X'))
    input_offset += 2;
  std::string cleaned;
  cleaned.reserve(raw.size());
  for (size_t i = input_offset; i < raw.size(); i++) {
    const char value = raw[i];
    if (value == '(')
      break;
    if (std::isxdigit(static_cast<unsigned char>(value)))
      cleaned.push_back(value);
  }
  if (cleaned.empty() || (cleaned.size() % 2) != 0 || cleaned.size() > MAX_PACKET_LEN * 2)
    return false;
  std::array<uint8_t, MAX_PACKET_LEN> parsed{};
  const size_t parsed_len = cleaned.size() / 2;
  for (size_t i = 0; i < parsed_len; i++) {
    const int high = hex_value(cleaned[i * 2]);
    const int low = hex_value(cleaned[i * 2 + 1]);
    if (high < 0 || low < 0)
      return false;
    parsed[i] = static_cast<uint8_t>((high << 4U) | low);
  }
  return this->from_raw(parsed.data(), parsed_len);
}

void AdvPacket::init(uint8_t ad_flag, uint8_t data_type) {
  this->bytes.fill(0);
  this->len = 0;
  this->ad_flag_index = MAX_PACKET_LEN;
  this->data_index = MAX_PACKET_LEN;
  if (ad_flag != 0) {
    this->ad_flag_index = 0;
    this->bytes[0] = 2;
    this->bytes[1] = AD_TYPE_FLAGS;
    this->bytes[2] = ad_flag;
    this->data_index = 3;
  } else {
    this->data_index = 0;
  }
  this->bytes[this->data_index + 1] = data_type;
}

bool AdvPacket::set_data_len(size_t data_len) {
  const size_t full_len = this->data_index + data_len + 2;
  if (data_len > 0xFE || full_len > MAX_PACKET_LEN)
    return false;
  this->bytes[this->data_index] = static_cast<uint8_t>(data_len + 1);
  this->len = full_len;
  return true;
}

uint8_t AdvPacket::ad_flag() const {
  return this->has_ad_flag() && this->ad_flag_index + 2 < this->len ? this->bytes[this->ad_flag_index + 2] : 0;
}
size_t AdvPacket::data_len() const {
  if (!this->has_data() || this->bytes[this->data_index] == 0)
    return 0;
  return this->bytes[this->data_index] - 1;
}
uint8_t AdvPacket::data_type() const {
  return this->has_data() && this->data_index + 1 < this->len ? this->bytes[this->data_index + 1] : 0;
}
uint8_t *AdvPacket::data() { return this->has_data() ? this->bytes.data() + this->data_index + 2 : nullptr; }
const uint8_t *AdvPacket::data() const {
  return this->has_data() ? this->bytes.data() + this->data_index + 2 : nullptr;
}

std::string AdvPacket::to_hex() const {
  std::string result;
  result.reserve(this->len * 3);
  char byte[4];
  for (size_t i = 0; i < this->len; i++) {
    std::snprintf(byte, sizeof(byte), i == 0 ? "%02X" : ".%02X", this->bytes[i]);
    result += byte;
  }
  return result;
}

bool AdvPacket::operator==(const AdvPacket &other) const {
  return this->len == other.len && std::equal(this->bytes.begin(), this->bytes.begin() + this->len, other.bytes.begin());
}

Encoder::Encoder(std::string encoding, std::string variant)
    : encoding_(std::move(encoding)), variant_(std::move(variant)) {}

void Encoder::set_ble_params(uint8_t ad_flag, uint8_t data_type) {
  this->ad_flag_ = ad_flag;
  this->data_type_ = data_type;
}
void Encoder::set_header(std::vector<uint8_t> header) { this->header_ = std::move(header); }
bool Encoder::matches_ble_params(const AdvPacket &packet) const {
  return packet.ad_flag() == this->ad_flag_ && packet.data_type() == this->data_type_;
}

bool Encoder::supports(const Command &command) const {
  if (command.type == CommandType::CUSTOM)
    return command.raw_cmd != 0;
  ControllerParams params;
  return !this->translate(command, params).empty();
}

std::vector<AdvPacket> Encoder::encode(const Command &command, ControllerParams &params) const {
  std::vector<Command> translated =
      command.type == CommandType::CUSTOM ? std::vector<Command>{command} : this->translate(command, params);
  std::vector<AdvPacket> packets;
  packets.reserve(translated.size());
  for (const auto &raw_command : translated) {
    params.tx_count++;
    AdvPacket packet;
    packet.init(this->ad_flag_, this->data_type_);
    const size_t data_len = this->header_.size() + this->payload_size_;
    if (data_len > MAX_PACKET_LEN || packet.data() == nullptr)
      continue;
    std::copy(this->header_.begin(), this->header_.end(), packet.data());
    this->encode_payload(packet.data() + this->header_.size(), raw_command, params);
    if (packet.set_data_len(data_len))
      packets.push_back(packet);
  }
  return packets;
}

bool Encoder::decode(const AdvPacket &packet, Command &command, ControllerParams &params) const {
  if (!packet.has_data() || packet.data_len() < this->header_.size())
    return false;
  if (packet.data_len() - this->header_.size() != this->payload_size_)
    return false;
  if (!std::equal(this->header_.begin(), this->header_.end(), packet.data()))
    return false;
  std::array<uint8_t, MAX_PACKET_LEN> payload{};
  std::copy(packet.data() + this->header_.size(), packet.data() + packet.data_len(), payload.begin());
  command = Command(CommandType::CUSTOM);
  return this->decode_payload(payload.data(), command, params);
}

uint16_t Encoder::random_seed(uint16_t forced_seed) {
  return forced_seed != 0 ? forced_seed : static_cast<uint16_t>(std::rand() % 0xFFF5U);
}

void Encoder::reverse_bits(uint8_t *data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    uint8_t value = data[i];
    value = static_cast<uint8_t>(((value & 0x55U) << 1U) | ((value & 0xAAU) >> 1U));
    value = static_cast<uint8_t>(((value & 0x33U) << 2U) | ((value & 0xCCU) >> 2U));
    data[i] = static_cast<uint8_t>(((value & 0x0FU) << 4U) | ((value & 0xF0U) >> 4U));
  }
}

void Encoder::whiten_lfsr(uint8_t *data, size_t len, uint8_t seed) {
  uint8_t state = seed;
  for (size_t i = 0; i < len; i++) {
    uint8_t mask = 0;
    for (size_t bit = 0; bit < 8; bit++) {
      state <<= 1U;
      if ((state & 0x80U) != 0) {
        state ^= 0x11U;
        mask |= 1U << bit;
      }
      state &= 0x7FU;
    }
    data[i] ^= mask;
  }
}

uint16_t Encoder::crc16_be(const uint8_t *data, size_t len, uint16_t seed, uint16_t polynomial) {
  uint16_t crc = seed;
  while (len-- > 0) {
    crc ^= static_cast<uint16_t>(*data++) << 8U;
    for (uint8_t bit = 0; bit < 8; bit++)
      crc = (crc & 0x8000U) != 0 ? static_cast<uint16_t>((crc << 1U) ^ polynomial)
                                 : static_cast<uint16_t>(crc << 1U);
  }
  return crc;
}

uint16_t Encoder::crc16_le(const uint8_t *data, size_t len, uint16_t seed, uint16_t polynomial, bool refin,
                           bool refout) {
  uint16_t crc = refin ? static_cast<uint16_t>(seed ^ 0xFFFFU) : seed;
  while (len-- > 0) {
    crc ^= *data++;
    for (uint8_t bit = 0; bit < 8; bit++)
      crc = (crc & 1U) != 0 ? static_cast<uint16_t>((crc >> 1U) ^ polynomial)
                            : static_cast<uint16_t>(crc >> 1U);
  }
  return refout ? static_cast<uint16_t>(crc ^ 0xFFFFU) : crc;
}

void Encoder::aes128_encrypt_block(const uint8_t key[16], const uint8_t input[16], uint8_t output[16]) {
  uint8_t expanded[176];
  uint8_t state[16];
  aes_expand_key(key, expanded);
  std::copy(input, input + 16, state);
  aes_add_round_key(state, expanded);
  for (size_t round = 1; round < 10; round++) {
    aes_sub_bytes(state);
    aes_shift_rows(state);
    aes_mix_columns(state);
    aes_add_round_key(state, expanded + round * 16);
  }
  aes_sub_bytes(state);
  aes_shift_rows(state);
  aes_add_round_key(state, expanded + 160);
  std::copy(state, state + 16, output);
}

MultiEncoder::MultiEncoder(std::string encoding, std::vector<Encoder *> encoders)
    : Encoder(std::move(encoding), "all"), encoders_(std::move(encoders)) {}

bool MultiEncoder::supports_any(const Command &command) const {
  return std::any_of(this->encoders_.begin(), this->encoders_.end(),
                     [&](const Encoder *encoder) { return encoder->supports(command); });
}

std::vector<AdvPacket> MultiEncoder::encode_all(const Command &command, ControllerParams &params) const {
  std::vector<AdvPacket> packets;
  uint8_t highest_tx_count = params.tx_count;
  for (const auto *encoder : this->encoders_) {
    ControllerParams copy = params;
    auto encoded = encoder->encode(command, copy);
    highest_tx_count = std::max(highest_tx_count, copy.tx_count);
    packets.insert(packets.end(), encoded.begin(), encoded.end());
  }
  params.tx_count = highest_tx_count;
  return packets;
}

}  // namespace ble_adv::protocol
