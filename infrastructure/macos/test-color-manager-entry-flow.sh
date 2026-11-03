#!/bin/bash
set -eu
root="$(cd "$(dirname "$0")/../.." && pwd)"
OPENGATE_EXPERIMENTAL_COLOR_MANAGER=1 \
    bash "$root/infrastructure/macos/test-native-entry-flow.sh" "$@"
