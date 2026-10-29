#!/bin/bash
set -eu
root="$(cd "$(dirname "$0")/../.." && pwd)"
UNBOUND_TEST_NAME=reliability-tests UNBOUND_TEST_SOURCE="$root/tests/platforms/macos/reliability-tests.cpp" \
    bash "$root/infrastructure/macos/test-panel-sessions.sh" "$@"
