#!/bin/bash
set -eu
root="$(cd "$(dirname "$0")/../.." && pwd)"
QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM=1 UNBOUND_TEST_HEADLESS=1 UNBOUND_TEST_NAME=panel-caller-flow-tests UNBOUND_TEST_SOURCE="$root/tests/platforms/macos/panel-caller-flow-tests.cpp" \
    bash "$root/infrastructure/macos/test-panel-sessions.sh" "$@"
