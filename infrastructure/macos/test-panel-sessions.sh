#!/bin/bash
set -eu
root="$(cd "$(dirname "$0")/../.." && pwd)"
qt="$root/build/macos/qtbase-source"
generated="$root/build/macos/qt-generated"
app="${1:-/Applications/DaVinci Resolve/DaVinci Resolve.app}"
out="$root/build/macos/${UNBOUND_TEST_NAME:-panel-session-tests}"
source_file="${UNBOUND_TEST_SOURCE:-$root/tests/platforms/macos/panel-session-tests.cpp}"
compile_flags=(-std=c++17 -g -Wall -Wextra -Werror)
if [[ "${UNBOUND_TEST_ASAN:-0}" == 1 ]]; then
    compile_flags+=(-fsanitize=address -fno-omit-frame-pointer)
    out="$out-asan"
fi
if [[ "${UNBOUND_TEST_OPTIMIZE:-0}" == 1 ]]; then
    compile_flags+=(-O2)
    out="$out-optimized"
fi
extra_sources=("$root/src/platforms/macos/nativechrome.mm" "$root/src/platforms/macos/nativekeyboardmenuroute.mm")
if [[ -n "${UNBOUND_TEST_MOC_HEADER:-}" ]]; then
    moc="$root/build/macos/test-tools/moc"
    if [[ ! -x "$moc" ]]; then python3 "$root/infrastructure/macos/build-test-moc.py"; fi
    metadata="$out-meta.cpp"
    "$moc" "$UNBOUND_TEST_MOC_HEADER" -o "$metadata"
    extra_sources+=("$metadata")
fi
xcrun clang++ "${compile_flags[@]}" -DUNBOUND_NATIVE_TESTS \
    -I "$generated" -I "$qt/include" -I "$qt/include/QtCore" -I "$qt/include/QtGui" -I "$qt/include/QtWidgets" \
    -I "$root/src/platforms/macos" "$source_file" "${extra_sources[@]}" \
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
