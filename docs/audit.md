# Глубокий аудит `ble_adv_controller`

## Назначение

Компонент эмулирует BLE-пульты китайских люстр и потолочных вентиляторов. ESP32 не устанавливает GATT-соединение, а циклически публикует advertising payload, который устройство воспринимает как команду пульта.

Полный путь данных:

```text
ESPHome entity/action
  -> high-level Command
  -> protocol registry
  -> vendor encoder
  -> AdvPacket (до 31 байта)
  -> общий GAP scheduler
  -> ESP-IDF raw advertising API
```

## Состояние legacy-реализации

До переработки один набор классов одновременно отвечал за:

- YAML/codegen;
- Home Assistant entities;
- mapping высокоуровневых команд;
- шифрование, CRC, whitening и bit order;
- очередь и тайминги BLE advertising;
- runtime-конфигурацию через динамические Number/Select;
- API services и decode diagnostics.

Это создавало жёсткую связность. Изменение внутреннего ESPHome API ломало код протокола, хотя сами wire formats не менялись.

Основные найденные проблемы:

1. Использовались устаревшие или внутренние API: `CustomAPIDevice`, ручная регистрация entities, старые light/fan schemas.
2. Протоколы зависели от ESPHome headers и не могли тестироваться обычным host compiler.
3. В текущей таблице конфигурации осталось 6 variants, хотя история репозитория содержала 15.
4. Не было единого declarative registry с metadata и лимитами ID.
5. Decode/re-encode существовал как runtime-инструмент, но не был regression test.
6. Несколько controllers могли конкурировать за глобальный ESP32 advertiser.
7. Отсутствовали CI, YAML smoke configs и проверки обеих framework.
8. Произвольная вложенная папка protocol не копируется механизмом local external components ESPHome; исходники должны лежать в корне компонента либо быть отдельным ESPHome component.

## Новая структура

### Protocol layer

Файлы `protocol*.h/.cpp` не включают ESPHome или ESP-IDF.

- `protocol.h/.cpp`: `Command`, `ControllerParams`, безопасный AD parser/builder, CRC, whitening, reverse bits, AES-128.
- `protocol_fanlamp.*`: FanLamp/LampSmart/remote/legacy algorithms.
- `protocol_zhijia.*`: MSC16, MSC26 и MSC26A.
- `protocol_registry.*`: 15 variants, lookup, `all`, decode и roundtrip diagnostics.

### Runtime layer

- `BleAdvController`: параметры одного логического пульта, command queue, deduplication и lifetime сообщения.
- `BleAdvHandler`: один общий владелец GAP advertiser, асинхронный state machine и round-robin packets.
- GAP transitions обрабатываются только по completion events: stop, raw-data configured, start, stop.
- После опустошения очереди стандартный ESPHome advertiser запускается снова.

### ESPHome layer

- актуальные `light_schema`, `fan_schema`, `button_schema`;
- `register_gap_event_handler`;
- actions с обязательным `synchronous=True`;
- явные BLE feature flags `USE_ESP32_BLE_ADVERTISING` и `USE_ESP32_BLE_UUID`;
- один handler совместно используется всеми controller instances.

## Протокольное покрытие

Registry содержит:

- `fanlamp_pro`: v1, v2, v3;
- `lampsmart_pro`: v1, v2, v3;
- `zhijia`: v0, v1, v2;
- `remote`: v1, v3;
- `other`: v1a, v1b, v2, v3.

Подробные AD flags, types, headers, ID limits и command mapping находятся в [protocol-matrix.md](protocol-matrix.md).

## Проверки

- Host C++ test компилируется с `-Wall -Wextra -Werror`.
- Все 15 variants проходят pair encode/decode/re-encode.
- Проверяется единичное увеличение `tx_count` для multi-variant encoding.
- Известный LampSmart Pro v3 packet проходит golden decode и `NO DIFF`.
- YAML проверен на ESPHome 2026.3.3, 2026.4.5 и 2026.5.2.
- Реально собраны ESP-IDF minimal и Arduino full firmware на ESPHome 2026.5.2.

## Оставшиеся риски

- Аппаратная валидация на физической ESP32 и реальном устройстве — чеклист в [hardware-validation.md](hardware-validation.md), заполняется вручную.
- Нет runtime unit test с mock GAP callbacks и симуляцией ошибок драйвера.
- Совместная работа с активными BLE tracker/server сценариями требует аппаратной проверки.

## Закрыто в текущей архитектуре

- Golden corpus в `tests/samples/` для top-3 encodings.
- Runtime tests: queue dedup, duration/max_duration, tx_count rollover.
- Diagnostic и config entities (Number/Select/text_sensor/sensor) при `show_config: true`.
- CI: protocol + runtime + yaml matrix + compile matrix + clang-format lint.
