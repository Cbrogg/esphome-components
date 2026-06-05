# Этап 1. Экстракция протокола BLE-люстр из legacy-компонента

## Контекст

Текущий компонент `ble_adv_controller` исторически совмещает две разные ответственности: reverse-engineered протокол BLE advertising для китайских люстр и устаревшую ESPHome-обвязку. В новой реализации нужно отделить протокольное ядро от ESPHome API, чтобы дальше строить актуальную интеграцию под ESPHome 2026.5.x без зависимости от старых `FAN_SCHEMA`, `rgb_light_schema`, `App.register_*`, `set_setup_priority` и прочих внутренних API.

В legacy-коде уже есть ценные части:

- `CommandType` с внутренними командами: `PAIR`, `UNPAIR`, `LIGHT_ON`, `LIGHT_OFF`, `LIGHT_DIM`, `LIGHT_CCT`, `LIGHT_WCOLOR`, `LIGHT_SEC_ON/OFF`, `FAN_ON/OFF`, `FAN_SPEED`, `FAN_ONOFF_SPEED`, `FAN_DIR`, `FAN_OSC`.
- `ControllerParam_t`: `id`, `tx_count`, `index`, `seed`.
- `BleAdvParam`: упаковка BLE advertising payload, AD flags, service/manufacturer data.
- `BleAdvEncoder`: базовая модель encode/decode/translate.
- Реализации протоколов: `fanlamp_pro`, `lampsmart_pro`, `zhijia`, `remote`, `other`.
- Для `fanlamp_pro` `PAIR` транслируется в протокольную команду `0x28`, `UNPAIR` в `0x45`, `LIGHT_ON/OFF` в `0x10/0x11`, `LIGHT_WCOLOR` в `0x21`, fan-команды в `0x31`, `0x15`, `0x16` и т.д.

## Цель

Выделить чистое C++ протокольное ядро, не завязанное на ESPHome entity/codegen API. На выходе должен быть модуль, который принимает высокоуровневую команду и параметры контроллера, а возвращает один или несколько готовых BLE advertising payload для отправки.

## Объём работ

1. Создать отдельный слой, условно:

```text
components/ble_adv_controller/protocol/
  command.h
  controller_params.h
  adv_packet.h
  encoder.h
  fanlamp_pro.*
  zhijia.*
  remote.*
  other.*
```

2. Сохранить все существующие vendor/variant encoder-реализации:

- `fanlamp_pro`: варианты `v1`, `v2`, `v3`.
- `lampsmart_pro`: варианты `v1`, `v2`, `v3`.
- `zhijia`: варианты `v0`, `v1`, `v2`.
- `remote`: варианты `v1`, `v3`.
- `other`: legacy-варианты `v1a`, `v1b`, `v2`, `v3`.

3. Зафиксировать таблицу соответствий:

```text
encoding -> variant -> encoder class -> constructor args -> BLE params -> header -> forced_id limits/defaults
```

4. Описать и покрыть протокольные команды:

- pairing/unpairing;
- light on/off;
- brightness;
- color temperature;
- cold/warm white combined command;
- secondary light;
- fan on/off;
- fan speed;
- fan direction;
- fan oscillation;
- custom/raw command.

5. Сделать protocol API без ESPHome entity-зависимостей:

```cpp
struct ControllerParams {
  uint32_t id;
  uint8_t tx_count;
  uint8_t index;
  uint16_t seed;
};

enum class CommandType { ... };

struct Command {
  CommandType type;
  uint8_t cmd;
  uint8_t args[4];
};

struct AdvPacket {
  uint8_t data[31];
  size_t len;
  uint32_t min_duration_ms;
};

class Encoder {
 public:
  virtual bool supports(const Command&) const = 0;
  virtual std::vector<AdvPacket> encode(const Command&, ControllerParams&) = 0;
  virtual bool decode(const AdvPacket&, Command&, ControllerParams&) = 0;
};
```

6. Сохранить decode/re-encode диагностику из legacy-кода, но вынести её в debug/helper, а не держать в HA service.

7. Добавить unit-like тесты или хотя бы standalone debug harness для:

- encode known command -> packet shape;
- decode known raw packet -> command/config;
- decode -> encode roundtrip;
- forced_id/index/tx_count behavior.

## Критерии готовности

- Протокольный слой компилируется отдельно от ESPHome entity APIs.
- Все vendor/variant definitions перенесены без потери функциональности.
- Можно программно вызвать `encode(PAIR)` для `fanlamp_pro/v3` и получить BLE payload с протокольной командой `0x28`.
- `tx_count` корректно инкрементируется внутри encoder flow.
- Есть таблица поддерживаемых протоколов и команд в README или docs.
- Нет зависимостей от `light`, `fan`, `button`, `number`, `select`, `api::CustomAPIDevice`, `App.register_*`, `setup_entity`.

## Замечания

ESPHome-обвязку не трогать на этом этапе, кроме минимального подключения новых файлов. Цель этапа — сохранить reverse-engineered протокол и убрать риск потерять рабочие алгоритмы при переписывании интеграции.
