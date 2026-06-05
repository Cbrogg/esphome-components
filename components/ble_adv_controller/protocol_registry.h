#pragma once

#include "protocol_fanlamp.h"
#include "protocol_zhijia.h"

#include <memory>

namespace ble_adv::protocol {

struct VariantDefinition {
  const char *encoding;
  const char *variant;
  uint32_t default_id;
  uint32_t max_id;
};

struct DecodeResult {
  Encoder *encoder{nullptr};
  Command command{};
  ControllerParams params{};
  AdvPacket reencoded{};
  bool roundtrip_equal{false};
};

class Registry {
 public:
  Registry();

  Encoder *find(const std::string &encoding, const std::string &variant) const;
  std::vector<Encoder *> variants(const std::string &encoding) const;
  std::vector<std::string> variant_names(const std::string &encoding) const;
  std::vector<AdvPacket> encode(const std::string &encoding, const std::string &variant, const Command &command,
                                ControllerParams &params) const;
  bool supports(const std::string &encoding, const std::string &variant, const Command &command) const;
  bool decode(const AdvPacket &packet, DecodeResult &result, bool ignore_ble_params = true) const;

  const std::vector<std::unique_ptr<Encoder>> &encoders() const { return this->encoders_; }

 private:
  template<typename T, typename... Args> T *add(Args &&...args) {
    auto encoder = std::make_unique<T>(std::forward<Args>(args)...);
    T *result = encoder.get();
    this->encoders_.push_back(std::move(encoder));
    return result;
  }

  std::vector<std::unique_ptr<Encoder>> encoders_;
};

const std::vector<VariantDefinition> &variant_definitions();
const VariantDefinition *find_definition(const std::string &encoding, const std::string &variant);

}  // namespace ble_adv::protocol
