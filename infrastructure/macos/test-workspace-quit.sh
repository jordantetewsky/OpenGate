#!/bin/bash
set -eu
root="$(cd "$(dirname "$0")/../.." && pwd)"
UNBOUND_TEST_NAME=workspace-quit-tests UNBOUND_TEST_SOURCE="$root/tests/platforms/macos/workspace-quit-tests.cpp" \
    UNBOUND_TEST_COMPILE_ONLY=1 bash "$root/infrastructure/macos/test-panel-sessions.sh" "$@"
if [[ "${UNBOUND_TEST_COMPILE_ONLY:-0}" != 1 ]]; then
    app="${1:-/Applications/DaVinci Resolve/DaVinci Resolve.app}"
    binary="$root/build/macos/workspace-quit-tests"
    if [[ "${UNBOUND_TEST_ASAN:-0}" == 1 ]]; then binary="$binary-asan"; fi
    if [[ "${UNBOUND_TEST_OPTIMIZE:-0}" == 1 ]]; then binary="$binary-optimized"; fi
    QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM=1 QT_PLUGIN_PATH="$app/Contents/PlugIns" "$binary"
    QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM=1 QT_PLUGIN_PATH="$app/Contents/PlugIns" "$binary" --floating
fi
