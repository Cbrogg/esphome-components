# Pairing Guide

## Стандартный pairing

1. Обесточьте светильник или вентилятор.
2. Включите питание.
3. В первые 5–10 секунд нажмите кнопку **Pair** в Home Assistant.
4. Проверьте on/off основного света.

## Если variant неизвестен

Временно установите в YAML:

```yaml
ble_adv_controller:
  - id: controller
    encoding: fanlamp_pro
    variant: all
```

`variant: all` отправляет команду всеми variants выбранного encoding. Используйте только для поиска рабочего варианта.

После успешного pairing:

1. Переключите `variant` на конкретное значение (`v1`, `v2`, `v3`).
2. Проверьте on/off и brightness.
3. Зафиксируйте рабочий variant в YAML.

## Настройка duration

Слишком короткий `duration` приводит к пропущенным командам. Проверка:

1. Быстро переключайте свет on/off 5–10 раз.
2. Если HA и реальное состояние расходятся — увеличьте `duration` (через YAML или config entity **Duration**).
3. Начните с `200ms`, увеличивайте шагами по 50ms.

Для длительного pairing увеличьте `max_duration` до `10s`.

## Pairing без кнопки

Если устройство уже спарено с телефоном или пультом:

1. Захватите advertising packet (nRF Connect, Wireshark).
2. Выполните action `ble_adv_controller.raw_decode`.
3. Перенесите `forced_id` и `index` в YAML.

Подробности: [CUSTOM.md](../components/ble_adv_controller/CUSTOM.md).

## Динамическая настройка в Home Assistant

При `show_config: true` (по умолчанию) доступны config entities:

- Encoding, Variant
- Duration, Max Duration
- Index, Forced ID

Diagnostic entities показывают TX counter, queue length и последний packet.
