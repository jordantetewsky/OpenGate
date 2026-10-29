#!/bin/bash
set -eu
root="$(cd "$(dirname "$0")/../.." && pwd)"
source "$root/infrastructure/macos/build-target.sh"
qt="${1:-$root/build/macos/qtbase-source}"
app="${2:-/Applications/DaVinci Resolve/DaVinci Resolve.app}"
out="$root/build/macos/ofx-probe/UnboundLoadingProbe.ofx.bundle/Contents/MacOS"
if [[ ! -f "$qt/src/corelib/global/qconfig.h" || ! -f "$qt/include/QtWidgets/QApplication" || ! -d "$out" ]]; then
    echo 'Configure Qt 5.15.2 headers and build the OpenFX probe first.' >&2
    exit 1
fi
generated="$root/build/macos/qt-generated"
mkdir -p "$generated/QtCore" "$generated/QtGui" "$generated/QtWidgets"
cp "$qt/src/corelib/global/qconfig.h" "$generated/QtCore/"
cp "$qt/src/corelib/qtcore-config.h" "$generated/QtCore/"
cp "$qt/src/gui/qtgui-config.h" "$generated/QtGui/"
cp "$qt/src/widgets/qtwidgets-config.h" "$generated/QtWidgets/"
revision="$(git -C "$root" rev-parse --short HEAD)"
if [[ -n "$(git -C "$root" status --porcelain)" ]]; then revision="$revision-dirty"; fi
OPENGATE_RESOLVE_APP="$app" python3 "$root/infrastructure/macos/build-manifest.py" "$generated"
objects="$root/build/macos/widget-probe-objects"
mkdir -p "$objects"
for source in "$root/tests/platforms/macos/qt-widget-probe.cpp" "$root/src/platforms/macos/nativechrome.mm"; do
    object="$objects/$(basename "$source").o"
    xcrun clang++ "${opengate_macos_flags[@]}" -std=c++17 -Wall -Wextra -Werror -g -fvisibility=hidden \
        -include "$generated/unboundbuildmanifest.h" \
        -DUNBOUND_BUILD_ID="\"$revision\"" \
        -I "$generated" -I "$qt/include" -I "$qt/include/QtCore" -I "$qt/include/QtGui" -I "$qt/include/QtWidgets" \
        -I "$root/src/platforms/macos" -c "$source" -o "$object"
done
xcrun clang++ "${opengate_macos_flags[@]}" -dynamiclib "$objects/qt-widget-probe.cpp.o" "$objects/nativechrome.mm.o" -framework AppKit -framework CoreGraphics \
    -L "$app/Contents/Frameworks" -lQt5Widgets.5 -lQt5Gui.5 -lQt5Core.5 \
    -Wl,-rpath,"$app/Contents/Frameworks" -o "$out/UnboundWidgetProbe.dylib"
xcrun dsymutil "$out/UnboundWidgetProbe.dylib" -o "$out/UnboundWidgetProbe.dylib.dSYM"
/usr/bin/codesign --force --sign - "$out/UnboundWidgetProbe.dylib"
# Resolve's scan cache tracks the OFX binary rather than its companion library.
touch "$out/UnboundLoadingProbe.ofx"
echo 'Built the read-only widget probe against Resolve’s Qt libraries.'
