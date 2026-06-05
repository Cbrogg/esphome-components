# Референсы по разработке ESPHome-компонентов

Проверено 5 июня 2026 года. Целевая версия реализации: ESPHome 2026.5.2.

## Официальная документация

- [External Components](https://esphome.io/components/external_components.html) — подключение local/git components, `components`, `refresh`.
- [Component Architecture](https://developers.esphome.io/architecture/components/) — разделение Python schema/codegen и C++ runtime.
- [ESPHome Developer Documentation](https://developers.esphome.io/) — актуальные архитектурные и migration материалы.
- [`register_action` requires `synchronous`](https://developers.esphome.io/blog/2026/03/12/register_action-now-requires-explicit-synchronous-parameter/) — обязательный параметр actions начиная с 2026.3.
- [ESPHome releases](https://github.com/esphome/esphome/releases) — changelog и migration notes.
- [ESPHome 2026.5.2 package](https://pypi.org/project/esphome/2026.5.2/) — зафиксированная версия, использованная для cross-compile.

## Референсный исходный код

- [`esp32_ble`](https://github.com/esphome/esphome/tree/2026.5.2/esphome/components/esp32_ble) — BLE parent, GAP handlers и advertising lifecycle.
- [`esp32_ble_beacon`](https://github.com/esphome/esphome/tree/2026.5.2/esphome/components/esp32_ble_beacon) — пример raw BLE advertising и feature flags.
- [`light`](https://github.com/esphome/esphome/tree/2026.5.2/esphome/components/light) — `LightOutput`, schemas и registration.
- [`fan`](https://github.com/esphome/esphome/tree/2026.5.2/esphome/components/fan) — `Fan`, traits, restore и control calls.
- [`button`](https://github.com/esphome/esphome/tree/2026.5.2/esphome/components/button) — schema и entity registration.

## Применённые правила

1. Python-файл описывает schema, IDs, зависимости и генерирует вызовы setters.
2. C++ runtime не создаёт Home Assistant entities динамически.
3. Общий аппаратный ресурс ESP32 представлен одним shared component.
4. Асинхронный GAP API управляется completion events, а не задержками.
5. Actions явно объявляют `synchronous=True`.
6. Feature flags включаются вместе: advertising требует UUID support в ESPHome 2026.5.x.
7. Wire protocol вынесен из ESPHome, чтобы тестироваться обычным C++ compiler.
