#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ble_adv::protocol {

constexpr size_t MAX_PACKET_LEN = 31;
constexpr size_t MAX_COMMAND_ARGS = 4;

enum class CommandType : uint8_t {
  NOCMD = 0,
  PAIR = 1,
  UNPAIR = 2,
  CUSTOM = 3,
  LIGHT_ON = 13,
  LIGHT_OFF = 14,
  LIGHT_DIM = 15,
  LIGHT_CCT = 16,
  LIGHT_WCOLOR = 17,
  LIGHT_SEC_ON = 18,
  LIGHT_SEC_OFF = 19,
  FAN_ON = 30,
  FAN_OFF = 31,
  FAN_SPEED = 32,
  FAN_ONOFF_SPEED = 33,
  FAN_DIR = 34,
  FAN_OSC = 35,
};

struct ControllerParams {
  uint32_t id{0};
  uint8_t tx_count{0};
  uint8_t index{0};
  uint16_t seed{0};
};

struct Command {
  explicit Command(CommandType command_type = CommandType::NOCMD) : type(command_type) {}

  CommandType type{CommandType::NOCMD};
  uint8_t raw_cmd{0};
  std::array<uint8_t, MAX_COMMAND_ARGS> args{};
};

struct AdvPacket {
  std::array<uint8_t, MAX_PACKET_LEN> bytes{};
  size_t len{0};
  uint32_t min_duration_ms{100};
  size_t ad_flag_index{MAX_PACKET_LEN};
  size_t data_index{MAX_PACKET_LEN};

  bool from_raw(const uint8_t *data, size_t data_len);
  bool from_hex(const std::string &raw);
  void init(uint8_t ad_flag, uint8_t data_type);
  bool set_data_len(size_t data_len);

  bool has_ad_flag() const { return this->ad_flag_index != MAX_PACKET_LEN; }
  uint8_t ad_flag() const;
  bool has_data() const { return this->data_index != MAX_PACKET_LEN; }
  size_t data_len() const;
  uint8_t data_type() const;
  uint8_t *data();
  const uint8_t *data() const;
  std::string to_hex() const;
  bool operator==(const AdvPacket &other) const;
};

class Encoder {
 public:
  Encoder(std::string encoding, std::string variant);
  virtual ~Encoder() = default;

  const std::string &encoding() const { return this->encoding_; }
  const std::string &variant() const { return this->variant_; }
  std::string id() const { return this->encoding_ + " - " + this->variant_; }

  void set_ble_params(uint8_t ad_flag, uint8_t data_type);
  void set_header(std::vector<uint8_t> header);
  bool matches_ble_params(const AdvPacket &packet) const;

  bool supports(const Command &command) const;
  std::vector<AdvPacket> encode(const Command &command, ControllerParams &params) const;
  bool decode(const AdvPacket &packet, Command &command, ControllerParams &params) const;

 protected:
  virtual std::vector<Command> translate(const Command &command, const ControllerParams &params) const = 0;
  virtual void encode_payload(uint8_t *payload, const Command &command, const ControllerParams &params) const = 0;
  virtual bool decode_payload(uint8_t *payload, Command &command, ControllerParams &params) const = 0;

  static uint16_t random_seed(uint16_t forced_seed);
  static void reverse_bits(uint8_t *data, size_t len);
  static void whiten_lfsr(uint8_t *data, size_t len, uint8_t seed);
  static uint16_t crc16_be(const uint8_t *data, size_t len, uint16_t seed, uint16_t polynomial = 0x1021);
  static uint16_t crc16_le(const uint8_t *data, size_t len, uint16_t seed, uint16_t polynomial = 0x8408,
                           bool refin = true, bool refout = true);
  static void aes128_encrypt_block(const uint8_t key[16], const uint8_t input[16], uint8_t output[16]);

  std::string encoding_;
  std::string variant_;
  uint8_t ad_flag_{0};
  uint8_t data_type_{0xFF};
  std::vector<uint8_t> header_;
  size_t payload_size_{0};
};

class MultiEncoder final : public Encoder {
 public:
  MultiEncoder(std::string encoding, std::vector<Encoder *> encoders);

  bool supports_any(const Command &command) const;
  std::vector<AdvPacket> encode_all(const Command &command, ControllerParams &params) const;

 protected:
  std::vector<Command> translate(const Command &, const ControllerParams &) const override { return {}; }
  void encode_payload(uint8_t *, const Command &, const ControllerParams &) const override {}
  bool decode_payload(uint8_t *, Command &, ControllerParams &) const override { return false; }

  std::vector<Encoder *> encoders_;
};

uint16_t read_le16(const uint8_t *data);
uint16_t read_be16(const uint8_t *data);
uint32_t read_le32(const uint8_t *data);
void write_le16(uint8_t *data, uint16_t value);
void write_be16(uint8_t *data, uint16_t value);
void write_le32(uint8_t *data, uint32_t value);

}  // namespace ble_adv::protocol
