#!/bin/bash
set -eu
root="$(cd "$(dirname "$0")/../.." && pwd)"
app="${1:-/Applications/DaVinci Resolve/DaVinci Resolve.app}"
helper="$root/build/macos/loading-probe/UnboundLoadingProbe.dylib"
if [[ ! -f "$helper" || ! -x "$app/Contents/MacOS/Resolve" ]]; then
    echo 'Build the loading probe and provide a valid Resolve app path first.' >&2
    exit 1
fi
# Never restart an editing session to run an experiment.
if ! processes="$(/bin/ps -axo comm=)"; then
    echo 'Cannot inspect running processes; refusing to start another Resolve.' >&2
    exit 1
fi
if printf '%s\n' "$processes" | /usr/bin/grep -q '/Contents/MacOS/Resolve$'; then
    echo 'Quit Resolve normally before running the startup probe.' >&2
    exit 1
fi
log="$(mktemp "$root/build/macos/loading-probe/resolve.XXXXXX")"
echo "Probe marker: $log"
echo 'An empty marker means loading was not demonstrated.'
UNBOUND_PROBE_LOG="$log" DYLD_INSERT_LIBRARIES="$helper" "$app/Contents/MacOS/Resolve"
