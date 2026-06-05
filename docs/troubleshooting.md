# Troubleshooting

## Свет не реагирует

1. Проверьте encoding и variant — см. [vendor-matrix.md](vendor-matrix.md).
2. Попробуйте `variant: all` для pairing.
3. Увеличьте `duration` и `max_duration`.
4. Проверьте логи ESPHome на `Unsupported command`.

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

1. Обновите компонент до версии с исправленным `ble_adv_handler` (raw callback + retry без потери пакетов).
2. Перепрошейте устройство.
3. В логе должны появиться `ADV_DATA_RAW_SET_COMPLETE` / `ADV_START_COMPLETE` без ошибок.
4. Queue Length должен возвращаться к 0 после команды.

## Предупреждения в логах

```
Component took a long time for an operation (56 ms)
```

Это связано с объёмом логов ESPHome, а не с ошибкой BLE. После настройки переключите logger на `INFO`.

## Decode/re-encode diff

Используйте action `ble_adv_controller.raw_decode` с захваченным packet. Результат `NO DIFF` подтверждает корректный encoding/variant/forced_id.

## Совместимость с другими BLE-компонентами

Компонент временно захватывает ESP32 advertiser. Одновременная работа с BLE tracker/server требует аппаратной проверки — см. [hardware-validation.md](hardware-validation.md).
