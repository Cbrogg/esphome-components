#pragma once

#include "ble_adv_controller.h"
#include "esphome/components/number/number.h"
#include "esphome/components/select/select.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/core/component.h"

namespace esphome::ble_adv_controller {

class BleAdvProtocolTextSensor : public text_sensor::TextSensor,
                                 public PollingComponent,
                                 public Parented<BleAdvController> {
 public:
  void update() override;
};

class BleAdvLastPacketTextSensor : public text_sensor::TextSensor,
                                   public PollingComponent,
                                   public Parented<BleAdvController> {
 public:
  void update() override;
};

class BleAdvTxCountSensor : public sensor::Sensor, public PollingComponent, public Parented<BleAdvController> {
 public:
  void update() override;
};

class BleAdvForcedIdSensor : public sensor::Sensor, public PollingComponent, public Parented<BleAdvController> {
 public:
  void update() override;
};

class BleAdvQueueLengthSensor : public sensor::Sensor, public PollingComponent, public Parented<BleAdvController> {
 public:
  void update() override;
};

class BleAdvDurationNumber : public number::Number, public Parented<BleAdvController> {
 public:
  void control(float value) override;
  void setup() override;
};

class BleAdvMaxDurationNumber : public number::Number, public Parented<BleAdvController> {
 public:
  void control(float value) override;
  void setup() override;
};

class BleAdvIndexNumber : public number::Number, public Parented<BleAdvController> {
 public:
  void control(float value) override;
  void setup() override;
};

class BleAdvForcedIdNumber : public number::Number, public Parented<BleAdvController> {
 public:
  void control(float value) override;
  void setup() override;
};

class BleAdvEncodingSelect : public select::Select, public Parented<BleAdvController> {
 public:
  void control(const std::string &value) override;
  void setup() override;
};

class BleAdvVariantSelect : public select::Select, public Parented<BleAdvController> {
 public:
  void control(const std::string &value) override;
  void setup() override;
};

}  // namespace esphome::ble_adv_controller
