#pragma once

#include "esphome/components/esp32_ble/ble.h"
#include "esphome/core/component.h"
#include "protocol_registry.h"

#include <list>
#include <vector>

namespace esphome::ble_adv_controller {

namespace protocol = ::ble_adv::protocol;

class BleAdvHandler : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_BLUETOOTH; }

  void set_ble_parent(esp32_ble::ESP32BLE *parent) { this->ble_parent_ = parent; }
  protocol::Registry &registry() { return this->registry_; }

  uint16_t add_packets(std::vector<protocol::AdvPacket> packets);
  void remove_packets(uint16_t message_id);
  size_t scheduled_queue_size() const { return this->packets_.size(); }
  const std::string &last_packet_hex() const { return this->last_packet_hex_; }
  bool decode_and_log(const protocol::AdvPacket &packet, bool ignore_ble_params = true);
  bool decode_hex_and_log(const std::string &raw);

  void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

 protected:
  enum class AdvertiserState : uint8_t {
    IDLE,
    ACQUIRING,
    CONFIGURING,
    STARTING,
    ADVERTISING,
    STOPPING,
  };

  struct ScheduledPacket {
    ScheduledPacket(uint16_t message_id, protocol::AdvPacket packet)
        : message_id(message_id), packet(std::move(packet)) {}

    uint16_t message_id;
    protocol::AdvPacket packet;
    bool processed{false};
    bool remove{false};
  };

  void configure_front_();
  void acquire_advertiser_();
  void request_stop_();
  void finish_front_();
  void restore_esphome_advertising_();

  protocol::Registry registry_;
  esp32_ble::ESP32BLE *ble_parent_{nullptr};
  std::list<ScheduledPacket> packets_;
  uint16_t next_message_id_{0};
  uint32_t packet_started_at_{0};
  AdvertiserState state_{AdvertiserState::IDLE};
  bool owns_advertiser_{false};
  std::string last_packet_hex_;

  esp_ble_adv_params_t advertising_params_{
      .adv_int_min = 0x20,
      .adv_int_max = 0x20,
      .adv_type = ADV_TYPE_NONCONN_IND,
      .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
      .peer_addr = {0, 0, 0, 0, 0, 0},
      .peer_addr_type = BLE_ADDR_TYPE_PUBLIC,
      .channel_map = ADV_CHNL_ALL,
      .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
  };
};

}  // namespace esphome::ble_adv_controller
