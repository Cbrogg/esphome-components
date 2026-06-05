# Этап 4. Интеграция с Home Assistant

## Статус

**Реализовано (CI). Аппаратный HA smoke — вне CI.**

Нативные `LightOutput`, `Fan` и `Button`. Diagnostic/config entities автоматически создаются при `show_config: true`.

### Критерии готовности

- [x] Diagnostic: protocol, variant, tx counter, device id, queue length, last packet (text_sensor/sensor)
- [x] Config Number/Select: duration, max_duration, encoding, variant, forced_id, index
- [x] `show_config` в YAML schema (default `true`)
- [x] `tests/yaml/ha-api-smoke.yaml` проходит `esphome config` в CI
- [x] Pairing Guide, Troubleshooting, Supported Devices docs
- [ ] Аппаратный smoke-тест с `api:` в Home Assistant — см. hardware-validation.md

## Цель
Предоставить полноценные Home Assistant сущности поверх нового протокольного ядра.

## Light Entity
Поддержать:
- on/off
- brightness
- color temperature
- transition
- state restore

Использовать актуальный LightOutput API ESPHome.

## Fan Entity
Поддержать:
- on/off
- speed
- direction
- oscillation
- restore state

Использовать актуальный Fan API ESPHome.

## Button Entity
Поддержать:
- pair
- unpair
- custom command

## Diagnostic Entities
Поддержать:
- protocol
- variant
- tx counter
- forced id
- queue length
- last packet

## Config Entities
Через Number/Select:
- duration
- max_duration
- encoding
- variant
- forced id
- index

Использовать только актуальные схемы ESPHome 2026.x.

## Home Assistant UX
Устройство должно отображаться как:
- Light
- Fan
- Buttons
- Diagnostics
- Configuration

## Документация
Подготовить:
- Quick Start
- Pairing Guide
- Troubleshooting
- Supported Devices

## Критерии готовности
- Устройство автоматически обнаруживается Home Assistant через ESPHome.
- Свет и вентилятор работают как нативные сущности.
- Настройки доступны через Configuration entities.
- Диагностика доступна через Diagnostic entities.
