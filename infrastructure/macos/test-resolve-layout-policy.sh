#!/bin/bash
set -eu
root="$(cd "$(dirname "$0")/../.." && pwd)"
QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM=1 UNBOUND_TEST_HEADLESS=1 UNBOUND_TEST_NAME=resolve-layout-policy-tests UNBOUND_TEST_SOURCE="$root/tests/platforms/macos/resolve-layout-policy-tests.cpp" \
    bash "$root/infrastructure/macos/test-panel-sessions.sh" "$@"
