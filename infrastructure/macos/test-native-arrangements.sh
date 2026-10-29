#!/bin/bash
set -eu
root="$(cd "$(dirname "$0")/../.." && pwd)"
UNBOUND_TEST_NAME=native-arrangement-tests UNBOUND_TEST_SOURCE="$root/tests/platforms/macos/native-arrangement-tests.cpp" \
    bash "$root/infrastructure/macos/test-panel-sessions.sh" "$@"
