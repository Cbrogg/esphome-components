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

## Предупреждения в логах

```
Component took a long time for an operation (56 ms)
```

Это связано с объёмом логов ESPHome, а не с ошибкой BLE. После настройки переключите logger на `INFO`.

## Decode/re-encode diff

Используйте action `ble_adv_controller.raw_decode` с захваченным packet. Результат `NO DIFF` подтверждает корректный encoding/variant/forced_id.

## Совместимость с другими BLE-компонентами

Компонент временно захватывает ESP32 advertiser. Одновременная работа с BLE tracker/server требует аппаратной проверки — см. [hardware-validation.md](hardware-validation.md).
