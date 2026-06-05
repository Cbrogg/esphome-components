# Этап 1. Экстракция протокола BLE-люстр из legacy-компонента

## Статус

**Реализовано.**

Протокольный слой вынесен в `protocol*.h/.cpp`, не зависит от ESPHome и содержит registry для всех 15 исторических variants.

### Критерии готовности

- [x] Per-variant command tests (encode всех supports()-команд + roundtrip)
- [x] forced_id/index mapping tests
- [x] tx_count increment test; rollover в `controller_logic.h` + runtime tests
- [x] Golden corpus в `tests/samples/` для top-3 encodings
- [x] `docs/protocol-matrix.md`

## Цель

Выделить из текущего `ble_adv_controller` чистое протокольное ядро для управления BLE-люстрами через advertising packets. Новый протокольный слой не должен зависеть от ESPHome entity/codegen API, Home Assistant сущностей, динамических `select/number`, `api::CustomAPIDevice` и устаревших внутренних методов ESPHome.

Этап считается успешным, если можно программно создать encoder для любого поддержанного vendor/variant, передать ему высокоуровневую команду и получить один или несколько готовых BLE advertising payload длиной до 31 байта.

## Почему это нужно

Текущий компонент смешивает:

- reverse-engineered протоколы китайских люстр;
- ESPHome codegen;
- Home Assistant entities;
- динамическую конфигурацию;
- API services;
- BLE advertising scheduler.

Из-за этого обновление ESPHome ломает всё сразу: `FAN_SCHEMA`, `rgb_light_schema`, `App.register_select`, `App.register_number`, `set_setup_priority`, `set_name`, `set_entity_category`, `register_component` и т.д.

Протокольная часть при этом является самой ценной и должна быть сохранена отдельно.

## Исходные артефакты

Из текущего компонента нужно извлечь и переосмыслить:

- `ble_adv_handler.h/.cpp`:
  - `CommandType`;
  - `Command`;
  - `ControllerParam_t`;
  - `BleAdvParam`;
  - `BleAdvEncoder`;
  - `BleAdvMultiEncoder`;
  - encode/decode helpers;
  - whitening/reverse helpers;
  - decode/re-encode diagnostics.
- `fanlamp_pro.h/.cpp`:
  - `FanLampEncoder`;
  - `FanLampEncoderV1`;
  - `FanLampEncoderV2`;
  - AES/signature/CRC/seed logic;
  - vendor command translation.
- `zhijia.*`.
- Все legacy variants из `__init__.py`:
  - `fanlamp_pro`;
  - `lampsmart_pro`;
  - `zhijia`;
  - `remote`;
  - `other`.

## Поддерживаемые протоколы и варианты

Перенести в явную таблицу registry:

```text
encoding        variants
fanlamp_pro     v1, v2, v3
lampsmart_pro   v1, v2, v3
zhijia          v0, v1, v2
remote          v1, v3
other           v1a, v1b, v2, v3
```

Для каждого варианта зафиксировать:

- encoder class;
- constructor args;
- BLE AD flag;
- BLE AD data type;
- protocol header;
- default variant;
- default forced id;
- max forced id;
- поддерживаемые команды;
- особенности pair/unpair;
- особенности fan/light команд;
- ограничения decode.

## Высокоуровневые команды

Сохранить текущую модель команд, но оформить её как стабильный protocol API:

```cpp
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
```

## Предлагаемый API протокольного слоя

```cpp
namespace ble_adv::protocol {

struct ControllerParams {
  uint32_t id{0};
  uint8_t tx_count{0};
  uint8_t index{0};
  uint16_t seed{0};
};

struct Command {
  CommandType type{CommandType::NOCMD};
  uint8_t raw_cmd{0};
  std::array<uint8_t, 4> args{0, 0, 0, 0};
};

struct AdvPacket {
  std::array<uint8_t, 31> bytes{};
  size_t len{0};
  uint32_t min_duration_ms{100};
};

class Encoder {
 public:
  virtual ~Encoder() = default;
  virtual const char *encoding() const = 0;
  virtual const char *variant() const = 0;
  virtual bool supports(const Command &cmd) const = 0;
  virtual std::vector<AdvPacket> encode(const Command &cmd, ControllerParams &params) = 0;
  virtual bool decode(const AdvPacket &packet, Command &cmd, ControllerParams &params) = 0;
};

}
```

## Обязательная диагностика

Сохранить decode/re-encode workflow:

1. принять raw advertising packet;
2. определить подходящий encoder;
3. decode в `Command + ControllerParams`;
4. вывести suggested config:

```yaml
ble_adv_controller:
  - id: my_controller
    encoding: fanlamp_pro
    variant: v3
    forced_id: 0x12345678
    index: 0
```

5. выполнить re-encode;
6. сравнить исходный packet и re-encoded packet;
7. вывести `NO DIFF` или подробный diff.

## Минимальные тестовые сценарии

Для каждого encoding/variant:

- encode pair;
- encode light_on/light_off;
- encode light_wcolor, если поддерживается;
- encode fan_onoff_speed, если поддерживается;
- encode fan_dir, если поддерживается;
- encode fan_osc, если поддерживается;
- проверить инкремент `tx_count`;
- проверить forced_id/index mapping;
- decode known packet, если есть sample;
- decode -> encode roundtrip.

## Критерии готовности

- Протокольный слой не зависит от `esphome.components.light`, `fan`, `button`, `select`, `number`.
- Протокольный слой не вызывает `App.register_*`, `setup_entity`, `register_component`, `CustomAPIDevice`.
- Для `fanlamp_pro/v3` команда `PAIR` транслируется в protocol command `0x28`.
- Для `fanlamp_pro` сохранена логика `UNPAIR = 0x45`, `LIGHT_ON = 0x10`, `LIGHT_OFF = 0x11`, `LIGHT_WCOLOR = 0x21`, fan-команд.
- Все vendor/variant definitions перенесены без потери данных.
- Есть README или таблица `docs/protocol-matrix.md`.

## Не входит в этап

- ESPHome YAML schema.
- Home Assistant entities.
- Реальная BLE отправка.
- OTA/web_server/API.
- Matter/Thread.

Этот этап должен дать чистую, проверяемую библиотеку протокола, поверх которой можно строить новый актуальный ESPHome-компонент.
