#!/bin/bash
set -eu
root="$(cd "$(dirname "$0")/../.." && pwd)"
UNBOUND_TEST_HEADLESS=1 UNBOUND_TEST_NAME=workspace-page-tests UNBOUND_TEST_SOURCE="$root/tests/platforms/macos/workspace-page-tests.cpp" \
    bash "$root/infrastructure/macos/test-panel-sessions.sh" "$@"
