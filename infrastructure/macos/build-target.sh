#!/bin/bash
# Source from Mac build entry points. Compilation and linkage use one target.
# This is a build capability, not a claim of live validation on the target OS.
OPENGATE_MACOS_DEPLOYMENT_TARGET="${OPENGATE_MACOS_DEPLOYMENT_TARGET:-11.0}"
if [[ ! "$OPENGATE_MACOS_DEPLOYMENT_TARGET" =~ ^[0-9]+\.[0-9]+(\.[0-9]+)?$ ]]; then
    echo 'OPENGATE_MACOS_DEPLOYMENT_TARGET must be a numeric macOS version.' >&2
    exit 1
fi
opengate_macos_flags=("-mmacosx-version-min=$OPENGATE_MACOS_DEPLOYMENT_TARGET")
OPENGATE_MACOS_ARCHS="${OPENGATE_MACOS_ARCHS:-universal}"
case "$OPENGATE_MACOS_ARCHS" in
    universal) opengate_macos_flags+=(-arch arm64 -arch x86_64); opengate_macos_architectures=(arm64 x86_64) ;;
    arm64|x86_64) opengate_macos_flags+=(-arch "$OPENGATE_MACOS_ARCHS"); opengate_macos_architectures=("$OPENGATE_MACOS_ARCHS") ;;
    *) echo 'OPENGATE_MACOS_ARCHS must be universal, arm64 or x86_64.' >&2; exit 1 ;;
esac
export OPENGATE_MACOS_DEPLOYMENT_TARGET OPENGATE_MACOS_ARCHS
