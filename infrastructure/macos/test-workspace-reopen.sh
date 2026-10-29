#!/bin/bash
set -eu
root="$(cd "$(dirname "$0")/../.." && pwd)"
app="${1:-/Applications/DaVinci Resolve/DaVinci Resolve.app}"
UNBOUND_TEST_COMPILE_ONLY=1 UNBOUND_TEST_NAME=workspace-reopen-tests UNBOUND_TEST_SOURCE="$root/tests/platforms/macos/workspace-reopen-tests.cpp" \
    bash "$root/infrastructure/macos/test-panel-sessions.sh" "$@"
if [[ "${UNBOUND_TEST_COMPILE_ONLY:-0}" == 1 ]]; then exit 0; fi
if pgrep -x Resolve >/dev/null; then
    echo 'NOT RUN: Resolve must be closed for the displayed two-process fixture.' >&2; exit 2
else
    status=$?; if [[ "$status" != 1 ]]; then exit 2; fi
fi
export QT_PLUGIN_PATH="$app/Contents/PlugIns"
for placement in floating attached; do
    folder="$(mktemp -d "$root/build/macos/reopen-fixture.$placement.XXXXXX")"
    export UNBOUND_WORKSPACE_FILE="$folder/Workspace.json"
    export UNBOUND_ARRANGEMENTS_FILE="$folder/Arrangements.json"
    export UNBOUND_SHORTCUTS_FILE="$folder/Shortcuts.json"
    export UNBOUND_DIAGNOSTICS_DIR="$folder/logs"
    "$root/build/macos/workspace-reopen-tests" write "$folder/Expected.json" "$placement"
    "$root/build/macos/workspace-reopen-tests" read "$folder/Expected.json" "$placement"
    echo "Cross-process evidence ($placement): $folder"
done
