#!/bin/bash
set -eu
root="$(cd "$(dirname "$0")/../.." && pwd)"
QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM=1 UNBOUND_TEST_HEADLESS=1 \
UNBOUND_TEST_NAME=timeline-content-viewport-tests \
UNBOUND_TEST_SOURCE="$root/tests/platforms/macos/timeline-content-viewport-tests.cpp" \
UNBOUND_TEST_MOC_HEADER="$root/tests/core/workspace-callers/timelineviewportfixtures.h" \
    bash "$root/infrastructure/macos/test-panel-sessions.sh" "$@"
