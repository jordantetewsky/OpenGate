#!/bin/bash
set -eu
root="$(cd "$(dirname "$0")/../.." && pwd)"
source "$root/infrastructure/macos/build-target.sh"
sdk='/Library/Application Support/Blackmagic Design/DaVinci Resolve/Developer'
out="$root/build/macos/io-helper"
bundle="$out/Unbound.dvcp.bundle/Contents/MacOS"
mkdir -p "$bundle" "$out/test-home/Library/Logs/OpenGate"
xcrun clang "${opengate_macos_flags[@]}" -Wall -Wextra -Werror -fvisibility=hidden -c \
    -I "$sdk/OpenFX/OpenFX-1.4/include" "$root/tests/platforms/macos/ofx-loading-probe.c" -o "$out/bootstrap.o"
xcrun clang++ "${opengate_macos_flags[@]}" -std=c++17 -Wall -Wextra -Werror -fvisibility=hidden -dynamiclib \
    -I "$sdk/CodecPlugin/Examples/x264_encoder_plugin/include" \
    "$root/src/platforms/macos/io-bootstrap.cpp" "$out/bootstrap.o" -o "$bundle/Unbound.dvcp"
mkdir -p "$bundle/../Resources"
# Build metadata is a resource, not a nested executable at Contents level.
if [[ -f "$bundle/../build-manifest.json" ]]; then
    mv "$bundle/../build-manifest.json" "$bundle/../Resources/build-manifest.json"
fi
cp "$root/build/macos/qt-generated/build-manifest.json" "$bundle/../Resources/build-manifest.json"
cp "$root/build/macos/ofx-probe/UnboundLoadingProbe.ofx.bundle/Contents/MacOS/UnboundWidgetProbe.dylib" "$bundle/"
/usr/bin/codesign --force --sign - "$bundle/Unbound.dvcp"
xcrun clang++ "${opengate_macos_flags[@]}" -std=c++17 -Wall -Wextra -Werror \
    -I "$sdk/CodecPlugin/Examples/x264_encoder_plugin/include" \
    "$root/tests/platforms/macos/io-bootstrap-tests.cpp" -o "$out/bootstrap-tests"
HOME="$out/test-home" "$out/bootstrap-tests" "$bundle/Unbound.dvcp"
