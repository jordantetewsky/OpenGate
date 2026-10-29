#!/bin/bash
set -eu
root="$(cd "$(dirname "$0")/../.." && pwd)"
OPENGATE_TEST_DISPLAYED_MIXER=1 OPENGATE_TEST_STACK_DROP=1 UNBOUND_TEST_NAME=mixer-mounted-tests UNBOUND_TEST_SOURCE="$root/tests/platforms/macos/opengate-first-open-tests.cpp" \
    bash "$root/infrastructure/macos/test-panel-sessions.sh" "$@"
