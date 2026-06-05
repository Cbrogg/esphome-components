# Этап 4. Интеграция с Home Assistant

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