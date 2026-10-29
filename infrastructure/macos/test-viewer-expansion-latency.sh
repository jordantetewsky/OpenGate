#!/bin/bash
set -eu
root="$(cd "$(dirname "$0")/../.." && pwd)"
UNBOUND_TEST_NAME=viewer-expansion-latency-tests UNBOUND_TEST_SOURCE="$root/tests/platforms/macos/viewer-expansion-latency-tests.cpp" \
    bash "$root/infrastructure/macos/test-panel-sessions.sh" "$@"
