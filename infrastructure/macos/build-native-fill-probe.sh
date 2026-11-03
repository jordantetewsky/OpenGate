#!/bin/bash
set -eu
root="$(cd "$(dirname "$0")/../.." && pwd)"
source "$root/infrastructure/macos/build-target.sh"
qt="$root/build/macos/qtbase-source"
generated="$root/build/macos/qt-generated"
app='/Applications/DaVinci Resolve/DaVinci Resolve.app'
out="${OPENGATE_NATIVE_FILL_PROBE_OUT:-$root/build/native-fill-proof}"
bundle="$out/OpenGateNativeFillProbe.ofx.bundle/Contents/MacOS"
if [[ -f "$bundle/UnboundWidgetProbe.dylib" ]] && pgrep -x Resolve >/dev/null; then
    echo 'Refusing to overwrite a probe bundle while Resolve is open; choose a new build output.' >&2
    exit 2
fi
sdk='/Library/Application Support/Blackmagic Design/DaVinci Resolve/Developer/OpenFX/OpenFX-1.4/include'
mkdir -p "$bundle"
xcrun clang "${opengate_macos_flags[@]}" -Wall -Wextra -Werror -fvisibility=hidden -bundle -I "$sdk" \
    "$root/tests/platforms/macos/ofx-loading-probe.c" -o "$bundle/OpenGateNativeFillProbe.ofx"
xcrun clang++ "${opengate_macos_flags[@]}" -std=c++17 -Wall -Wextra -Werror -O2 -g -fvisibility=hidden -dynamiclib \
    -I "$generated" -I "$qt/include" -I "$qt/include/QtCore" -I "$qt/include/QtGui" -I "$qt/include/QtWidgets" \
    -I "$root/src/platforms/macos" "$root/tests/platforms/macos/native-fill-probe.cpp" "$root/src/platforms/macos/nativechrome.mm" \
    -framework AppKit -framework CoreGraphics -L "$app/Contents/Frameworks" -lQt5Widgets.5 -lQt5Gui.5 -lQt5Core.5 \
    -Wl,-rpath,"$app/Contents/Frameworks" -o "$bundle/UnboundWidgetProbe.dylib"
/usr/bin/codesign --force --sign - "$bundle/UnboundWidgetProbe.dylib"
/usr/bin/codesign --force --sign - "$bundle/OpenGateNativeFillProbe.ofx"
echo "Built development-only paint probe in $out; nothing installed or launched."
