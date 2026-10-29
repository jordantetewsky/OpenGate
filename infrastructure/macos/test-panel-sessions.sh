#!/bin/bash
set -eu
root="$(cd "$(dirname "$0")/../.." && pwd)"
qt="$root/build/macos/qtbase-source"
generated="$root/build/macos/qt-generated"
app="${1:-/Applications/DaVinci Resolve/DaVinci Resolve.app}"
out="$root/build/macos/${UNBOUND_TEST_NAME:-panel-session-tests}"
source_file="${UNBOUND_TEST_SOURCE:-$root/tests/platforms/macos/panel-session-tests.cpp}"
xcrun clang++ -std=c++17 -g -Wall -Wextra -Werror -DUNBOUND_NATIVE_TESTS \
    -I "$generated" -I "$qt/include" -I "$qt/include/QtCore" -I "$qt/include/QtGui" -I "$qt/include/QtWidgets" \
    -I "$root/src/platforms/macos" "$source_file" "$root/src/platforms/macos/nativechrome.mm" \
    -L "$app/Contents/Frameworks" -lQt5Widgets.5 -lQt5Gui.5 -lQt5Core.5 \
    -framework AppKit -framework CoreGraphics -Wl,-rpath,"$app/Contents/Frameworks" -o "$out"
if [[ "${UNBOUND_TEST_COMPILE_ONLY:-0}" != 1 ]]; then
    if [[ "${UNBOUND_TEST_HEADLESS:-0}" != 1 ]]; then
        if pgrep -x Resolve >/dev/null; then
            echo "NOT RUN: Resolve is open. Native-window fixtures require Resolve closed; compilation succeeded." >&2
            exit 2
        else
            process_status=$?
            if [[ "$process_status" != 1 ]]; then
                echo "NOT RUN: unable to verify that Resolve is closed; compilation succeeded." >&2
                exit 2
            fi
        fi
    fi
    QT_PLUGIN_PATH="$app/Contents/PlugIns" "$out"
fi
