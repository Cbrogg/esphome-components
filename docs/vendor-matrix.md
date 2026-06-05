# Vendor matrix

Соответствие мобильных приложений, пультов и encoding/variant в `ble_adv_controller`.

| Приложение / источник | Encoding | Рекомендуемый variant | Примечания |
|---|---|---|---|
| FanLamp Pro | `fanlamp_pro` | `v3` | Свет + вентилятор, direction/oscillation |
| ApplianceSmart | `fanlamp_pro` | `v3` | Тот же wire format, что FanLamp Pro |
| LampSmart Pro | `lampsmart_pro` | `v3` | Отличается signature prefix в V2/V3 |
| LampSmart Pro - Soft Lighting | `lampsmart_pro` | `v3` | |
| Vmax Smart | `lampsmart_pro` | `v3` | |
| Zhi Jia | `zhijia` | `v2` | MSC26A; fan только на v2 |
| Известные BLE-пульты | `remote` | `v3` | v1 без AD flag |
| Legacy FanLamp / ControlSwitch | `other` | `v1b` | Backward compatibility |

## Поиск variant

1. Временно установите `variant: all`.
2. Выполните Pair в первые секунды после включения питания.
3. Проверьте on/off основного света.
4. Переберите конкретные variants и зафиксируйте рабочий в YAML.

## forced_id и index

Если устройство уже спарено с телефоном или пультом, можно не выполнять Pair:

- захватите advertising packet через nRF Connect или Wireshark;
- используйте action `ble_adv_controller.raw_decode`;
- перенесите `forced_id` и `index` в YAML контроллера.
