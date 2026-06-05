# BLE ADV ESPHome Components

Актуальный внешний компонент ESPHome для управления BLE-люстрами и вентиляторами через advertising packets.

Поддерживаются семейства приложений и пультов:

- FanLamp Pro / ApplianceSmart;
- LampSmart Pro / Vmax Smart;
- Zhi Jia;
- известные remote-варианты;
- legacy-варианты `other`.

Компонент разделён на независимое протокольное ядро, BLE scheduler и нативные ESPHome-платформы. Целевая версия: ESPHome 2026.5.2+.

Документация:

- [Quick Start](components/ble_adv_controller/README.md)
- [Глубокий аудит](docs/audit.md)
- [Архитектура](docs/architecture.md)
- [Матрица протоколов](docs/protocol-matrix.md)
- [Vendor matrix](docs/vendor-matrix.md)
- [Compatibility matrix](docs/compatibility-matrix.md)
- [Pairing Guide](docs/pairing-guide.md)
- [Troubleshooting](docs/troubleshooting.md)
- [Supported Devices](docs/supported-devices.md)
- [Hardware Validation](docs/hardware-validation.md)
- [Официальные ESPHome-референсы](docs/esphome-development-reference.md)
- [Roadmap](docs/roadmap/)
- [Техническое описание протоколов](components/ble_adv_controller/CUSTOM.md)

## Проверка

```bash
sh tests/run_protocol_tests.sh
sh tests/run_runtime_tests.sh
esphome config tests/yaml/minimal-idf.yaml
esphome config tests/yaml/full-arduino.yaml
esphome config tests/yaml/ha-api-smoke.yaml
```

## Credits

Основано на reverse engineering и исходных работах MasterDevX, flicker581, aronsky, 14roiron, NicoIIT и участников сообщества Home Assistant.
