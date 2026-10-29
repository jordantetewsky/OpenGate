#!/bin/bash
set -eu
root="$(cd "$(dirname "$0")/../.." && pwd)"
UNBOUND_TEST_NAME=native-workspace-ui-tests UNBOUND_TEST_SOURCE="$root/tests/platforms/macos/native-workspace-ui-tests.cpp" \
    bash "$root/infrastructure/macos/test-panel-sessions.sh" "$@"
