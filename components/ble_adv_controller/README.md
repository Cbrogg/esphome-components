# ble_adv_controller

Компонент отправляет BLE advertising packets для управления совместимыми потолочными светильниками и вентиляторами. Обратной связи от устройства нет: состояния ESPHome являются предполагаемыми.

## Установка

```yaml
external_components:
  - source: github://Cbrogg/esphome-components
    components: [ble_adv_controller]
```

Требуются ESP32 и ESPHome 2026.5.2+.

## Минимальная конфигурация

```yaml
ble_adv_controller:
  - id: controller
    encoding: fanlamp_pro
    variant: v3
```

## Полная конфигурация

```yaml
ble_adv_controller:
  - id: controller
    encoding: fanlamp_pro
    variant: v3
    forced_id: 0x12345678
    index: 0
    duration: 200ms
    max_duration: 3s
    seq_duration: 100ms
    reversed: false
    show_config: true

light:
  - platform: ble_adv_controller
    id: main_light
    name: Main Light
    ble_adv_controller_id: controller
    cold_white_color_temperature: 167 mireds
    warm_white_color_temperature: 333 mireds
    min_brightness: 1%
    constant_brightness: false
    separate_dim_cct: false

  - platform: ble_adv_controller
    name: Secondary Light
    ble_adv_controller_id: controller
    secondary: true

fan:
  - platform: ble_adv_controller
    name: Ceiling Fan
    ble_adv_controller_id: controller
    speed_count: 6
    use_direction: true
    use_oscillation: false
    forced_refresh_on_start: true

button:
  - platform: ble_adv_controller
    name: Pair
    ble_adv_controller_id: controller
    cmd: pair
```

## Варианты

| Encoding | Variants | Default |
|---|---|---|
| `fanlamp_pro` | `v1`, `v2`, `v3`, `all` | `v3` |
| `lampsmart_pro` | `v1`, `v2`, `v3`, `all` | `v3` |
| `zhijia` | `v0`, `v1`, `v2`, `all` | `v2` |
| `remote` | `v1`, `v3`, `all` | `v3` |
| `other` | `v1a`, `v1b`, `v2`, `v3`, `all` | `v1b` |

`all` отправляет команду всеми вариантами выбранного encoding. Используйте его только для поиска рабочего варианта и pairing.

## Actions

```yaml
then:
  - ble_adv_controller.pair: controller
  - ble_adv_controller.unpair: controller
  - ble_adv_controller.command:
      id: controller
      command: 16
      arg0: 0
      arg1: 0
      arg2: 0
      arg3: 0
  - ble_adv_controller.raw_inject:
      id: controller
      raw: "02.01.19..."
  - ble_adv_controller.raw_decode:
      id: controller
      raw: "02.01.19..."
```

`raw_decode` выводит найденный encoding/variant, параметры контроллера, suggested YAML и результат decode/re-encode.

## Diagnostic и config entities

При `show_config: true` (по умолчанию) автоматически создаются:

- **Diagnostic:** Protocol, Variant, Transmissions, Device ID (если `show_config: false`), Last Packet и Command Queue (скрыты по умолчанию)
- **Config:** Advertising Duration, Max Advertising Duration, Device Index, Device ID, Protocol, Variant

При `show_config: false` остаются только основные diagnostic entities без config-редакторов.

Опциональный `name:` у контроллера задаёт префикс для diagnostic/config сущностей — удобно при нескольких контроллерах на одном ESP:

```yaml
ble_adv_controller:
  - id: fan_light_controller
    name: "Люстра с вентилятором"
    encoding: fanlamp_pro
    show_config: false
```

## Pairing

1. Обесточьте светильник.
2. Включите питание.
3. В первые несколько секунд нажмите Pair.
4. Если вариант неизвестен, временно используйте `variant: all`.
5. После успешного pairing переберите конкретные варианты и зафиксируйте рабочий.

Если известны `forced_id` и `index` существующего приложения или пульта, pairing не требуется.

## Ограничения

- BLE advertising односторонний, фактическое состояние устройства не читается.
- Слишком короткий `duration` приводит к пропущенным командам.
- Частые light transitions создают очередь; по умолчанию transition отключён.
- Одновременное использование других raw BLE advertisers требует аппаратной проверки.
