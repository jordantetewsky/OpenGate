#!/bin/bash
set -eu
root="$(cd "$(dirname "$0")/../.." && pwd)"
UNBOUND_TEST_NAME=panel-group-tests UNBOUND_TEST_SOURCE="$root/tests/platforms/macos/panel-group-tests.cpp" \
    bash "$root/infrastructure/macos/test-panel-sessions.sh" "$@"
