#!/bin/sh
set -eu

ROOT="$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

CXX="${CXX:-c++}"
"$CXX" -std=c++17 -Wall -Wextra -Werror -pedantic \
  components/ble_adv_controller/protocol.cpp \
  components/ble_adv_controller/protocol_fanlamp.cpp \
  components/ble_adv_controller/protocol_zhijia.cpp \
  components/ble_adv_controller/protocol_registry.cpp \
  tests/generate_samples.cpp \
  -o /tmp/ble_adv_generate_samples
/tmp/ble_adv_generate_samples
