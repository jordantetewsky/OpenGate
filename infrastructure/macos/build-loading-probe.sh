#!/bin/bash
set -eu
root="$(cd "$(dirname "$0")/../.." && pwd)"
out="$root/build/macos/loading-probe"
mkdir -p "$out"
xcrun clang -Wall -Wextra -Werror -dynamiclib \
    "$root/tests/platforms/macos/loading-probe.c" -o "$out/UnboundLoadingProbe.dylib"
xcrun clang -Wall -Wextra -Werror \
    "$root/tests/platforms/macos/probe-host.c" -o "$out/probe-host"
log="$(mktemp "$out/self-test.XXXXXX")"
UNBOUND_PROBE_LOG="$log" DYLD_INSERT_LIBRARIES="$out/UnboundLoadingProbe.dylib" "$out/probe-host"
if ! /usr/bin/grep -q '^Unbound loading probe: pid=' "$log"; then
    echo 'FAIL: helper did not load in the test host.' >&2
    exit 1
fi
echo "PASS: helper loaded in the test host. Evidence: $log"
echo 'Resolve loading remains unvalidated.'
