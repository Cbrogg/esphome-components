# Supported Devices

Компонент управляет устройствами через BLE advertising (без GATT-соединения). Обратной связи от устройства нет.

## Поддерживаемые приложения

| Приложение | Encoding | Default variant |
|---|---|---|
| FanLamp Pro | `fanlamp_pro` | `v3` |
| ApplianceSmart | `fanlamp_pro` | `v3` |
| LampSmart Pro | `lampsmart_pro` | `v3` |
| LampSmart Pro - Soft Lighting | `lampsmart_pro` | `v3` |
| Vmax Smart | `lampsmart_pro` | `v3` |
| Zhi Jia | `zhijia` | `v2` |
| Legacy FanLamp / ControlSwitch | `other` | `v1b` |

## Пульты

Известные BLE-пульты: encoding `remote`, variant `v3` (или `v1`).

## Возможности по типу устройства

| Устройство | Light | Fan | Direction | Oscillation |
|---|---|---|---|---|
| FanLamp Pro combo | да | да | да | v2/v3 |
| LampSmart Pro lamp | да | нет | — | — |
| Zhi Jia lamp | да | v2 only | нет | нет |

Полная матрица: [compatibility-matrix.md](compatibility-matrix.md).

## Не поддерживается

- RGB-люстры
- Устройства без BLE advertising (только GATT/BLE connect)
- Matter / Thread

Если ваше приложение не в списке, но генерирует advertising packets — используйте `raw_decode` и откройте issue с захваченным packet.
