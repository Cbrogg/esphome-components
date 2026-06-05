# Troubleshooting

## Свет не реагирует

1. Проверьте encoding и variant — см. [vendor-matrix.md](vendor-matrix.md).
2. Попробуйте `variant: all` для pairing.
3. Увеличьте `duration` и `max_duration`.
4. Проверьте логи ESPHome на `Unsupported command`.

### В логах нет `ble_adv_controller.light`, но кнопка pair и fan работают

Это значит, что `write_state()` не вызывается — команда из HA не доходит до output.

1. Перепрошейте устройство с актуальным компонентом (`external_components` → `local` path или свежий git). Output light **не** регистрируется как отдельный component.
2. Если в логе `OTA rollback detected! Rolled back from partition 'app1'` — устройство откатилось на старую прошивку. Залейте снова (`esphome upload`) и дайте ему поработать **60+ секунд** без перезагрузки.
3. При старте для каждой лампы: `[I][ble_adv_controller.light]: Ready: '…' parent=0x…` и `[C][light]: Light '…'`.
4. При переключении в HA — `update_state`, `Switch ON/OFF`, `LIGHT_WCOLOR`, `Queue packet`.
5. Проверьте **entity_id**: для `id: fan_light` в YAML это `light.fan_light` («Люстра с вентилятором»), не старые сущности вроде «Люстра спальня».
6. Обход HA: кнопки **DEBUG Light ON/OFF** — в serial должны быть `Press cmd=13/14` и `Queue packet`.
7. Developer Tools → Services:
   ```yaml
   service: light.turn_on
   target:
     entity_id: light.fan_light
   ```
   Если логи light появились — проблема в сцене/группе HA, не в прошивке.

## Cold/warm перепутаны

Добавьте в контроллер:

```yaml
ble_adv_controller:
  - id: controller
    reversed: true
```

## Zhi Jia: brightness или CCT не работают

Включите раздельные команды:

```yaml
light:
  - platform: ble_adv_controller
    separate_dim_cct: true
```

## Минимальная яркость слишком высокая

Уменьшайте `min_brightness` в YAML или через HA до момента, когда свет реально гаснет.

## Fan direction/oscillation не работают

Direction и oscillation поддерживаются только FanLamp-family encodings (`fanlamp_pro`, `lampsmart_pro`, `remote`, `other`). Для `zhijia` установите:

```yaml
fan:
  - platform: ble_adv_controller
    use_direction: false
    use_oscillation: false
```

## `esp_ble_gap_config_adv_data_raw failed: ESP_ERR_INVALID_STATE`

Пакеты формируются (TX Counter растёт, Last Packet меняется), но в эфир не уходят. Причина — конфликт с рекламой `esp32_ble`.

Что делать:

1. Обновите компонент до версии без `USE_ESP32_BLE_ADVERTISING` и без `advertising_register_raw_advertisement_callback` — handler использует GAP напрямую, как legacy-код.
2. В YAML явно отключите рекламу ESPHome, если блок `esp32_ble` присутствует: `advertising: false`.
3. Перепрошейте устройство.
4. В логе должно появиться `Advertising started` (event-driven GAP) без `esp_ble_gap_config_adv_data_raw failed`.
5. Queue Length должен возвращаться к 0 после команды.

## HA показывает «недоступно», в serial — `CONNECTION_CLOSED`

1. Строки `ESPHome Logs … CONNECTION_CLOSED` от **192.168.1.x** — это обрыв **просмотра логов** (`esphome logs` / IDE), не разрыв Home Assistant. HA в логе: `Home Assistant … connected` / `disconnected`.
2. Слабый Wi‑Fi (ниже −70 dBm) даёт задержки API — улучшите сигнал или перенесите ESP ближе к роутеру.
3. Уровень `logger: DEBUG` и `show_config: true` увеличивают нагрузку — для эксплуатации используйте `level: INFO` и `show_config: false`.
4. Частые команды света (ползунок яркости) грузят BLE-очередь — в прошивке есть throttle 200 ms между промежуточными шагами; финальное значение всё равно уходит.

## Предупреждения в логах

```
Component took a long time for an operation (56 ms)
```

Это связано с объёмом логов ESPHome, а не с ошибкой BLE. После настройки переключите logger на `INFO`.

## Decode/re-encode diff

Используйте action `ble_adv_controller.raw_decode` с захваченным packet. Результат `NO DIFF` подтверждает корректный encoding/variant/forced_id.

## Совместимость с другими BLE-компонентами

Компонент временно захватывает ESP32 advertiser. Одновременная работа с BLE tracker/server требует аппаратной проверки — см. [hardware-validation.md](hardware-validation.md).
