#!/bin/sh
set -eu

CXX="${CXX:-c++}"
"$CXX" -std=c++17 -Wall -Wextra -Werror -pedantic \
  components/ble_adv_controller/protocol.cpp \
  components/ble_adv_controller/protocol_fanlamp.cpp \
  components/ble_adv_controller/protocol_zhijia.cpp \
  components/ble_adv_controller/protocol_registry.cpp \
  tests/runtime_test.cpp \
  -o /tmp/ble_adv_runtime_tests
/tmp/ble_adv_runtime_tests
