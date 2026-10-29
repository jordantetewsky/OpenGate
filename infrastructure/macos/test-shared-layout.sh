#!/bin/bash
set -eu
root="$(cd "$(dirname "$0")/../.." && pwd)"
UNBOUND_TEST_HEADLESS=1 UNBOUND_TEST_NAME=shared-layout-tests UNBOUND_TEST_SOURCE="$root/tests/core/workspace-callers/layout.cpp" \
    bash "$root/infrastructure/macos/test-panel-sessions.sh" "$@"
