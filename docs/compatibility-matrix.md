# Compatibility matrix

Совместимость ESPHome-сущностей и опций YAML с protocol layer.

## Light

| Возможность | fanlamp_pro | lampsmart_pro | zhijia v0/v1 | zhijia v2 | remote/other |
|---|---|---|---|---|---|
| on/off | да | да | да | да | да |
| brightness + CCT (LIGHT_WCOLOR) | да | да | v1+ | да | да |
| separate_dim_cct | нет | нет | да | да | нет |
| secondary light | да | да | да | да | да |
| reversed (cold/warm) | да | да | да | да | да |
| RGB | нет | нет | нет | нет | нет |

## Fan

| Возможность | fanlamp_pro | lampsmart_pro | zhijia v2 | remote/other |
|---|---|---|---|---|
| on/off | да | да | да | да |
| speed 3/6 | да | да | только 6 | да |
| direction | да | да | **нет** | да |
| oscillation | v2/v3 | v2/v3 | **нет** | v2/v3 |
| forced_refresh_on_start | да | да | да | да |

YAML-валидация запрещает `use_direction` и `use_oscillation` для encoding `zhijia`.

## Button / actions

| Команда | Все FanLamp-family | zhijia v0 | zhijia v1 | zhijia v2 |
|---|---|---|---|---|
| pair / unpair | да | да | да | да |
| light_on / light_off | да | да | да | да |
| light_dim / light_cct | нет | да | да | да |
| fan_on / fan_off / fan_speed | нет | нет | нет | да |
| fan_dir / fan_osc | да | нет | нет | нет |
| custom / raw_inject / raw_decode | да | да | да | да |

## Controller options

| Опция | Поведение |
|---|---|
| `variant: all` | Отправка всеми variants encoding; только для pairing/поиска |
| `duration` | Минимальное время рекламы одной команды |
| `max_duration` | Максимум, если очередь пуста |
| `seq_duration` | Интервал ротации payload внутри одной команды |
| `forced_id` | Статический identifier вместо FNV1 от `id` |
| `index` | Дополнительный счётчик для нескольких устройств |

Подробные wire offsets и command codes: [protocol-matrix.md](protocol-matrix.md).
