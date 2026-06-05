# Матрица протоколов

## Variants

| Encoding | Variant | Encoder | AD flag | AD type | Header | ID limit |
|---|---|---|---:|---:|---|---:|
| fanlamp_pro | v1 | FanLamp V1 | 0x19 | 0x03 | 77 F8 | 0xFFFFFF |
| fanlamp_pro | v2 | FanLamp V2 | 0x19 | 0x03 | F0 08 | 0xFFFFFFFF |
| fanlamp_pro | v3 | FanLamp V2 + AES signature | 0x19 | 0x03 | F0 08 | 0xFFFFFFFF |
| lampsmart_pro | v1 | FanLamp V1 | 0x19 | 0x03 | 77 F8 | 0xFFFFFF |
| lampsmart_pro | v2 | FanLamp V2 | 0x19 | 0x03 | F0 08 | 0xFFFFFFFF |
| lampsmart_pro | v3 | FanLamp V2 + AES signature | 0x19 | 0x03 | F0 08 | 0xFFFFFFFF |
| zhijia | v0 | MSC16 | 0x1A | 0xFF | F9 08 49 | 0xFFFF |
| zhijia | v1 | MSC26 | 0x1A | 0xFF | F9 08 49 | 0xFFFFFF |
| zhijia | v2 | MSC26A | 0x1A | 0xFF | 22 9D | 0xFFFFFF |
| remote | v1 | FanLamp V1 | none | 0xFF | 56 55 18 87 52 | 0xFFFFFF |
| remote | v3 | FanLamp V2 + AES signature | 0x02 | 0x16 | F0 08 | 0xFFFFFFFF |
| other | v1a | FanLamp V1 legacy | 0x02 | 0x03 | 77 F8 | 0xFFFFFF |
| other | v1b | FanLamp V1 legacy | 0x02 | 0x16 | F9 08 | 0xFFFFFF |
| other | v2 | FanLamp V2 legacy | 0x19 | 0x16 | F0 08 | 0xFFFFFFFF |
| other | v3 | FanLamp V2 + AES signature | 0x19 | 0x16 | F0 08 | 0xFFFFFFFF |

## Высокоуровневые команды

| Команда | FanLamp V1/V2 | ZhiJia v0 | ZhiJia v1 | ZhiJia v2 |
|---|---:|---:|---:|---:|
| pair | 0x28 | 0xB4 | 0xA2 | 0xA2 |
| unpair | 0x45 | 0xB0 | 0xA3 | 0xA3 |
| light_on | 0x10 | 0xB3 | 0xA5 | 0xA5 |
| light_off | 0x11 | 0xB2 | 0xA6 | 0xA6 |
| light_dim | - | 0xB5 | 0xAD | 0xAD |
| light_cct | - | 0xB7 | 0xAE | 0xAE |
| light_wcolor | 0x21 | - | 0xA8 | 0xA8 |
| secondary on/off | 0x12/0x13 | 0xA6 | 0xAF/0xB0 | 0xAF/0xB0 |
| fan on/off | 0x31 | - | - | 0xD2/0xD3 |
| fan speed | 0x31/0x32 | - | - | 0xDC..0xE1 |
| fan direction | 0x15 | - | - | - |
| fan oscillation | 0x16 | - | - | - |

Все variants поддерживают raw custom command на уровне соответствующего encoder.
