#!/bin/bash
set -eu
root="$(cd "$(dirname "$0")/../.." && pwd)"
source "$root/infrastructure/macos/build-target.sh"
qt="${1:-$root/build/macos/qtbase-source}"
app="${2:-/Applications/DaVinci Resolve/DaVinci Resolve.app}"
out="${OPENGATE_WIDGET_PROBE_OUT:-$root/build/macos/ofx-probe/UnboundLoadingProbe.ofx.bundle/Contents/MacOS}"
development_flags=(-std=c++17)
if [[ "${OPENGATE_INPUT_CUSTOMIZATION_PREVIEW:-0}" == 1 ]]; then
    development_flags+=(-DOPENGATE_INPUT_CUSTOMIZATION_PREVIEW)
fi
if [[ "${OPENGATE_DEVELOPMENT_ENTRY:-0}" == 1 ]]; then
    development_flags+=(-DOPENGATE_DEVELOPMENT_ENTRY)
fi
if [[ -n "${OPENGATE_COLOR_VISUAL_TRIAL_DIRECTORY:-}" ]]; then
    [[ "$OPENGATE_COLOR_VISUAL_TRIAL_DIRECTORY" == /* && -f "$OPENGATE_COLOR_VISUAL_TRIAL_DIRECTORY/profile/Colors.json" ]] || {
        echo 'The explicit local color trial requires an absolute directory with profile/Colors.json.' >&2
        exit 1
    }
    trial_literal="$(python3 -c 'import json,sys; print(json.dumps(sys.argv[1]))' "$OPENGATE_COLOR_VISUAL_TRIAL_DIRECTORY")"
    development_flags+=("-DOPENGATE_COLOR_VISUAL_TRIAL_DIRECTORY=$trial_literal")
fi
if [[ -n "${OPENGATE_NATIVE_COLOR_PROBE_HEADER:-}" ]]; then
    [[ -n "${OPENGATE_COLOR_VISUAL_TRIAL_DIRECTORY:-}" && "$OPENGATE_NATIVE_COLOR_PROBE_HEADER" == /* && -f "$OPENGATE_NATIVE_COLOR_PROBE_HEADER" ]] || {
        echo 'The native color probe requires an explicit local trial and absolute scratch header.' >&2
        exit 1
    }
    probe_literal="$(python3 -c 'import json,sys; print(json.dumps(sys.argv[1]))' "$OPENGATE_NATIVE_COLOR_PROBE_HEADER")"
    development_flags+=("-DOPENGATE_NATIVE_COLOR_PROBE_HEADER=$probe_literal")
fi
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
OPENGATE_RESOLVE_APP="$app" OPENGATE_WIDGET_OPTIMIZATION=O2 python3 "$root/infrastructure/macos/build-manifest.py" "$generated"
objects="$root/build/macos/widget-probe-objects"
mkdir -p "$objects"
for source in "$root/tests/platforms/macos/qt-widget-probe.cpp" "$root/src/platforms/macos/nativechrome.mm"; do
    object="$objects/$(basename "$source").o"
    xcrun clang++ "${opengate_macos_flags[@]}" "${development_flags[@]}" -Wall -Wextra -Werror -O2 -g -fvisibility=hidden \
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
if [[ -f "$out/UnboundLoadingProbe.ofx" ]]; then touch "$out/UnboundLoadingProbe.ofx"; fi
echo 'Built the read-only widget probe against Resolve’s Qt libraries.'
