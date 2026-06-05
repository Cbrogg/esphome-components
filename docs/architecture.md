# Архитектура ble_adv_controller

## Назначение

Компонент преобразует высокоуровневые команды света и вентилятора в vendor-specific BLE advertising payload длиной до 31 байта и управляет их повторной отправкой через ESP32 BLE GAP.

## Слои

### Protocol layer

Файлы `protocol*.h/.cpp` не зависят от ESPHome, Home Assistant или ESP-IDF.

Основные типы:

- `CommandType` — стабильный набор высокоуровневых команд;
- `Command` — команда и четыре аргумента;
- `ControllerParams` — identifier, index, tx counter и deterministic seed;
- `AdvPacket` — безопасный parser/builder BLE AD sections;
- `Encoder` — translate, encode, decode;
- `Registry` — все vendor/variant definitions и автоматический decode.

Алгоритмы FanLamp и ZhiJia используют явные byte offsets. Packed-структуры, unaligned casts и зависимость от endian платформы исключены. AES-128 ECB для FanLamp v3 реализован внутри protocol layer, поэтому host tests не требуют ESP-IDF.

### Runtime layer

`BleAdvController`:

- хранит параметры конкретного управляемого устройства;
- кодирует команды;
- удаляет устаревшие команды того же типа;
- выдерживает `duration` и `max_duration`;
- передаёт пакеты общему scheduler.

`BleAdvHandler`:

- единственный владелец BLE advertiser;
- обрабатывает GAP events;
- последовательно проходит состояния `CONFIGURING`, `STARTING`, `ADVERTISING`, `STOPPING`;
- чередует несколько payload с `seq_duration`;
- восстанавливает стандартный ESPHome advertiser после очистки очереди.

### ESPHome layer

Python codegen использует публичные API ESPHome 2026.5:

- `cg.register_component`;
- `esp32_ble.register_gap_event_handler`;
- `light.light_schema` / `light.register_light`;
- `fan.fan_schema` / `fan.register_fan`;
- `button.button_schema` / `button.new_button`;
- `automation.register_action(..., synchronous=True)`.

Legacy `CustomAPIDevice`, `App.register_*`, ручной `setup_entity` и динамически создаваемые незарегистрированные entities не используются.

## Поток команды

```text
ESPHome entity/action
  -> BleAdvController::enqueue
  -> Registry::encode
  -> controller queue/deduplication
  -> BleAdvHandler scheduler
  -> esp_ble_gap_config_adv_data_raw
  -> GAP completion event
  -> esp_ble_gap_start_advertising
```

## Диагностика decode

```text
raw hex
  -> AdvPacket parser
  -> Registry tries encoders
  -> Command + ControllerParams
  -> re-encode with the decoded seed
  -> payload comparison
  -> suggested YAML + NO DIFF/diff
```

## Проверки

- host protocol tests для всех 15 variants;
- known packet golden test для `lampsmart_pro/v3`;
- roundtrip для каждого encoder;
- YAML smoke configs для ESP-IDF и Arduino;
- firmware compile для обеих framework-конфигураций.
