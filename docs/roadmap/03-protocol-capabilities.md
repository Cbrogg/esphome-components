# Этап 3. Полная реализация возможностей протокола

## Статус

**Реализовано (CI). Аппаратная валидация — вне CI.**

Runtime API полный: свет, вентилятор, pairing, raw inject/decode, decode/re-encode diagnostics.

### Критерии готовности

- [x] Golden corpus в `tests/samples/` для fanlamp_pro, lampsmart_pro, zhijia
- [x] `docs/vendor-matrix.md`
- [x] `docs/compatibility-matrix.md`
- [ ] Аппаратная валидация — чеклист в `docs/hardware-validation.md` (заполняется вручную)

## Цель
Покрыть все возможности поддерживаемых протоколов без привязки к Home Assistant.

## Свет
Поддержать:
- on/off
- brightness
- color temperature
- cold white
- warm white
- combined white
- secondary light

## Вентилятор
Поддержать:
- on/off
- speed
- direction
- oscillation
- forced refresh

## Pairing
Поддержать:
- pair
- unpair
- forced id
- index
- tx counter

## Raw режим
Поддержать:
- raw inject
- raw decode
- decode and suggest config

## Конфигурация
Поддержать:
- encoding
- variant
- duration
- max_duration
- reversed
- forced_id
- index

## Автотесты
Для каждого encoder:
- encode
- decode
- roundtrip
- packet validation

## Документация
Подготовить:
- protocol matrix
- command matrix
- vendor matrix
- compatibility matrix

## Критерии готовности
- Все команды legacy-компонента реализованы.
- Нет потери функциональности относительно старого проекта.
- Все протоколы покрыты тестами.
