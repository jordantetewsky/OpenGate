#!/bin/bash
set -eu
root="$(cd "$(dirname "$0")/../.." && pwd)"
source "$root/infrastructure/macos/build-target.sh"
sdk="${1:-/Library/Application Support/Blackmagic Design/DaVinci Resolve/Developer/OpenFX/OpenFX-1.4/include}"
out="$root/build/macos/ofx-probe"
bundle="$out/UnboundLoadingProbe.ofx.bundle"
mkdir -p "$bundle/Contents/MacOS"
xcrun clang "${opengate_macos_flags[@]}" -Wall -Wextra -Werror -fvisibility=hidden -bundle -I "$sdk" \
    "$root/tests/platforms/macos/ofx-loading-probe.c" \
    -o "$bundle/Contents/MacOS/UnboundLoadingProbe.ofx"
/usr/bin/codesign --force --sign - "$bundle/Contents/MacOS/UnboundLoadingProbe.ofx"
xcrun clang "${opengate_macos_flags[@]}" -Wall -Wextra -Werror "$root/tests/platforms/macos/ofx-probe-host.c" -o "$out/probe-host"
# Isolate the host-test log from evidence produced by Resolve.
mkdir -p "$out/test-home/Library/Logs/OpenGate"
HOME="$out/test-home" "$out/probe-host" "$bundle/Contents/MacOS/UnboundLoadingProbe.ofx"
echo "PASS: OpenFX discovery entry points. Bundle: $bundle"
