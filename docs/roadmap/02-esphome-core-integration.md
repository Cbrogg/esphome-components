# Этап 2. Базовая интеграция с ESPHome 2026.5.x

## Цель
Создать новый компонент, полностью совместимый с ESPHome 2026.5.2+, использующий только актуальные публичные API ESPHome.

## Статус

**Реализовано.**

Protocol layer, общий GAP scheduler, YAML controller, native actions и smoke-конфигурации для ESP-IDF/Arduino. CI compile на ESPHome 2026.5.2 и 2026.4.5.

## Требования
- Полный отказ от устаревших API:
  - FAN_SCHEMA
  - rgb_light_schema
  - App.register_*
  - setup_entity
  - CustomAPIDevice services
- Использовать актуальный codegen ESPHome.
- Использовать ESP32 BLE advertising API из актуальной версии.
- Поддерживать Arduino и ESP-IDF framework.

## Архитектура
Слой протокола из этапа 1 подключается как библиотека.

Компонент состоит из:
- protocol layer
- advertising scheduler
- ESPHome codegen
- runtime component

## YAML MVP
```yaml
ble_adv_controller:
  - id: controller
    encoding: fanlamp_pro
    variant: v3
```

## Первые действия
Поддержать только:
- pair
- unpair
- raw command
- send packet

Без Light/Fan/HA сущностей.

## BLE Scheduler
Отдельный runtime объект:
- очередь команд
- duration
- max_duration
- повтор рекламы
- отмена дубликатов

## Диагностика
Логи:
- encoder
- packet
- queue
- advertiser

## Критерии готовности
- Компонент собирается на ESPHome 2026.5.2.
- Не использует deprecated API.
- Команда Pair приводит к отправке валидного BLE advertising пакета.
- Работает на ESP32 без Home Assistant.
