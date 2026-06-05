# Этап 5. Стратегия тестирования и контроль качества

## Статус

**Реализовано (CI). Hardware playbook — шаблон для ручного заполнения.**

### Критерии готовности

- [x] Protocol tests: encode/decode/roundtrip, golden samples, command matrix
- [x] Runtime tests: queue dedup, duration logic, tx_count rollover
- [x] Golden corpus в `tests/samples/`
- [x] YAML smoke: minimal-idf, full-arduino, all-encodings, zhijia-fan, config-entities, ha-api-smoke
- [x] Compile matrix: ESPHome 2026.5.2 + 2026.4.5
- [x] YAML matrix: ESPHome 2026.3.3 / 2026.4.5 / 2026.5.2
- [x] Lint job (clang-format) в CI
- [x] `docs/hardware-validation.md`
- [ ] Заполненный hardware checklist на реальном устройстве

## Цель

Обеспечить возможность безопасно развивать компонент без повторения ситуации, когда обновление ESPHome ломает десятки мест одновременно и это обнаруживается только на этапе ручной проверки на реальной люстре.

Основной принцип:

```text
Каждая найденная ошибка должна приводить к появлению нового теста.
```

## Пирамида тестирования

Проект должен содержать несколько уровней тестов.

### Уровень 1. Протокольные тесты

Тестируется только protocol layer.

Без ESPHome.
Без Home Assistant.
Без BLE драйвера.

Проверяется:

- encode;
- decode;
- roundtrip;
- packet validation;
- tx counter;
- forced_id;
- index;
- variant-specific behavior.

### Уровень 2. ESPHome Runtime тесты

Проверяется:

- создание компонента;
- регистрация сущностей;
- конфигурация YAML;
- codegen;
- runtime initialization.

Без реального BLE.

### Уровень 3. Интеграционные тесты

Проверяется:

- protocol -> scheduler;
- scheduler -> advertiser;
- queue behavior;
- duration/max_duration;
- command deduplication;
- reconnect behavior.

### Уровень 4. Аппаратные тесты

Проверяется:

- реальная ESP32;
- реальные люстры;
- реальные BLE advertising пакеты.

Эти тесты не должны быть обязательными для CI.

## Unit тесты протокола

Для каждого encoder и variant должны существовать тесты:

### Pairing

```text
pair -> packet
packet -> decode
```

### Lighting

```text
light_on
light_off
brightness 0
brightness 1
brightness 50
brightness 100
warm white
cold white
mixed white
```

### Fan

```text
fan_on
fan_off
speed levels
fan direction
fan oscillation
```

### Custom

```text
custom command
raw packet decode
```

## Golden Tests

Для известных устройств сохранить набор эталонных пакетов.

Пример:

```text
samples/
  fanlamp_pro_v3/
    pair.bin
    light_on.bin
    fan_speed_3.bin
```

Тесты должны проверять:

```text
encode(command) == sample
```

Это позволит обнаруживать случайные изменения протокола.

## Roundtrip Tests

Для всех известных пакетов:

```text
decode(packet)
encode(result)
compare(packet)
```

Результат:

```text
NO DIFF
```

или подробный diff.

## Совместимость ESPHome

Нужны тесты сборки минимум на:

```text
ESPHome latest
ESPHome latest-1
ESPHome latest-2
```

Например:

```text
2026.5.x
2026.4.x
2026.3.x
```

Задача — максимально рано замечать API изменения.

## YAML Smoke Tests

Должны существовать минимальные конфиги:

```yaml
ble_adv_controller:
```

```yaml
ble_adv_controller:
light:
```

```yaml
ble_adv_controller:
fan:
```

```yaml
ble_adv_controller:
button:
```

которые автоматически прогоняются через:

```bash
esphome config
esphome compile
```

в CI.

## Regression Tests

Каждый баг из истории проекта должен фиксироваться тестом.

Особенно:

- FAN_SCHEMA migration;
- rgb_light_schema migration;
- register_select migration;
- register_number migration;
- custom_services migration;
- Component registration migration;
- Light API migration;
- Fan API migration.

## Покрытие

Минимальные цели:

```text
protocol layer       >= 90%
ESPHome glue layer   >= 70%
critical encoders    100% основных сценариев
```

## CI Pipeline

Каждый Pull Request должен запускать:

1. lint;
2. build;
3. unit tests;
4. protocol tests;
5. yaml smoke tests;
6. compatibility matrix.

## Критерии готовности

- Есть автоматические тесты для всех encoder variants.
- Есть golden packets.
- Есть roundtrip тесты.
- Есть smoke-сборка YAML конфигов.
- Обновление ESPHome не может сломать проект незаметно.
- Любой новый баг сопровождается новым тестом.
