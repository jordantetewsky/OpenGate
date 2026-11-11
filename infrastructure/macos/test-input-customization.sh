#!/bin/bash
set -eu
root="$(cd "$(dirname "$0")/../.." && pwd)"
export OPENGATE_INPUT_PREVIEW_SNAPSHOT="${OPENGATE_INPUT_PREVIEW_SNAPSHOT:-$root/build/macos/input-customization-preview.png}"
QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM=1 UNBOUND_TEST_HEADLESS=1 \
UNBOUND_TEST_NAME=input-customization-tests \
UNBOUND_TEST_SOURCE="$root/tests/platforms/macos/input-customization-tests.cpp" \
    bash "$root/infrastructure/macos/test-panel-sessions.sh" "$@"
