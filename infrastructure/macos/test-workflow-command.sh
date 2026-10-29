#!/bin/bash
set -eu
root="$(cd "$(dirname "$0")/../.." && pwd)"
python3 "$root/infrastructure/macos/build-workflow-commands.py"
QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM=1 UNBOUND_TEST_HEADLESS=1 UNBOUND_TEST_NAME=workflow-command-tests UNBOUND_TEST_SOURCE="$root/tests/platforms/macos/workflow-command-tests.cpp" \
    bash "$root/infrastructure/macos/test-panel-sessions.sh" "$@"
