#!/bin/bash
set -eu
root="$(cd "$(dirname "$0")/../.." && pwd)"
source "$root/infrastructure/macos/build-target.sh"
out="$root/build/macos/OpenGate.app/Contents"
mkdir -p "$out/MacOS"
launcher_slices=()
for architecture in "${opengate_macos_architectures[@]}"; do
    slice="$root/build/macos/OpenGate-$architecture"
    xcrun swiftc -O -target "$architecture-apple-macosx$OPENGATE_MACOS_DEPLOYMENT_TARGET" "$root/src/platforms/macos/launcher.swift" -o "$slice"
    launcher_slices+=("$slice")
done
xcrun lipo -create "${launcher_slices[@]}" -output "$out/MacOS/OpenGate"
cat > "$out/Info.plist" <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
<key>CFBundleExecutable</key><string>OpenGate</string>
<key>CFBundleIdentifier</key><string>com.cinewaltz.opengate</string>
<key>CFBundleName</key><string>OpenGate</string>
<key>CFBundlePackageType</key><string>APPL</string>
<key>CFBundleShortVersionString</key><string>0.1.0</string>
<key>CFBundleVersion</key><string>1</string>
<key>NSHighResolutionCapable</key><true/>
</dict></plist>
PLIST
python3 - "$out/Info.plist" "$OPENGATE_MACOS_DEPLOYMENT_TARGET" <<'PYINFO'
import plistlib,sys
from pathlib import Path
path=Path(sys.argv[1]);data=plistlib.loads(path.read_bytes());data['LSMinimumSystemVersion']=sys.argv[2];path.write_bytes(plistlib.dumps(data))
PYINFO
/usr/bin/codesign --force --sign - "$root/build/macos/OpenGate.app"
testdir="$(mktemp -d "$root/build/macos/launcher-test.XXXXXX")"
UNBOUND_LAUNCH_DIRECTORY="$testdir" "$out/MacOS/OpenGate" --self-test
mkdir -p "$testdir/workflow-home"
HOME="$testdir/workflow-home" python3 "$root/src/platforms/macos/OpenGate.py"
test -s "$testdir/workflow-home/Library/Logs/OpenGate/show-controls.request"
echo 'PASS: Workflow Integration launch request'
