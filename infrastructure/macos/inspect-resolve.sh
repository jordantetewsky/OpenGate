#!/bin/bash
# Read-only prerequisites for the loading experiment; does not launch Resolve.
set -eu

app="${1:-/Applications/DaVinci Resolve/DaVinci Resolve.app}"
binary="$app/Contents/MacOS/Resolve"
if [[ ! -f "$binary" ]]; then
    echo "Resolve executable not found: $binary" >&2
    exit 1
fi

echo '== Host =='
/usr/bin/sw_vers
/usr/bin/uname -m
echo '== Resolve version =='
/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$app/Contents/Info.plist"
/usr/libexec/PlistBuddy -c 'Print :CFBundleVersion' "$app/Contents/Info.plist"
echo '== Executable architectures =='
/usr/bin/lipo -archs "$binary"
echo '== Linked libraries =='
/usr/bin/otool -L "$binary"
echo '== Signing flags =='
/usr/bin/codesign -dvvv "$app" 2>&1
echo '== Entitlements (failure leaves loading permissions unresolved) =='
if ! /usr/bin/codesign -d --entitlements - --arch "$(/usr/bin/uname -m)" "$app" 2>&1; then
    echo 'Entitlements could not be decoded; do not infer permission to load a helper.'
fi
echo '== Signature verification =='
if ! /usr/bin/codesign --verify --strict --verbose=2 "$app" 2>&1; then
    echo 'Signature verification failed; investigate before a loading experiment.'
fi
echo 'Inspection complete. No loading mechanism has been tested.'
