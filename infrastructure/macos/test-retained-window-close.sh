#!/bin/bash
set -eu
root="$(cd "$(dirname "$0")/../.." && pwd)"
OPENGATE_TEST_RETAINED_CLOSE=1 bash "$root/infrastructure/macos/test-opengate-first-open.sh" "$@"
