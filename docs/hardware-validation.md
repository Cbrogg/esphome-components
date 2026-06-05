# Hardware Validation

Аппаратные тесты не входят в CI. Этот чеклист предназначен для ручной проверки перед релизом.

## Требования

- ESP32 с Bluetooth (любая dev board)
- Совместимая люстра или вентилятор (минимум 2 семейства: FanLamp + ZhiJia)
- Home Assistant с ESPHome integration
- nRF Connect (Android/iOS) для захвата advertising packets

## Прошивка

```bash
esphome run tests/yaml/full-arduino.yaml
```

Или пользовательский конфиг на базе [ha-api-smoke.yaml](../tests/yaml/ha-api-smoke.yaml).

## Чеклист

### BLE advertising

- [ ] Pair выполняется в окне после включения питания
- [ ] Light on/off синхронизируется с HA (при стабильном `duration`)
- [ ] Brightness и color temperature работают
- [ ] Fan speed меняется (для combo-устройства)
- [ ] Direction/oscillation работают (FanLamp-family)

### Decode diagnostics

- [ ] Захваченный packet декодируется через `ble_adv_controller.raw_decode`
- [ ] Re-encode показывает `NO DIFF` для рабочего конфига
- [ ] Suggested YAML содержит корректные encoding/variant/forced_id

### Scheduler

- [ ] Быстрые последовательные команды не вызывают зависание
- [ ] После опустошения очереди ESPHome BLE advertiser восстанавливается
- [ ] Diagnostic **Queue Length** возвращается к 0

### Home Assistant

- [ ] Устройство обнаруживается автоматически через `api:`
- [ ] Light, Fan, Button отображаются как нативные сущности
- [ ] Config entities (Duration, Variant) изменяют поведение без перепрошивки
- [ ] Diagnostic entities обновляют TX counter и Last Packet

### Совместимость

- [ ] FanLamp Pro / LampSmart Pro: encoding `fanlamp_pro` или `lampsmart_pro`, variant `v3`
- [ ] Zhi Jia: encoding `zhijia`, variant `v2`, fan без direction/oscillation

## Протокол регрессии

Каждый найденный баг:

1. Захватить raw packet.
2. Добавить golden sample в `tests/samples/`.
3. Добавить тест в `tests/protocol_test.cpp`.
4. Зафиксировать шаги воспроизведения в [troubleshooting.md](troubleshooting.md).

## Статус валидации

| Устройство | Encoding/variant | Дата | Результат |
|---|---|---|---|
| _заполнить при тестировании_ | | | |
