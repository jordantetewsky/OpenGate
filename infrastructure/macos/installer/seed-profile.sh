#!/bin/bash
set -eu
home=$1; user=$2; resources=$3
base="$home/Library/Application Support/Unbound"
if [[ ! -e "$base/Workspace.json" && ! -e "$base/Arrangements.json" && ! -e "$base/Profile.json" ]]; then
 /bin/mkdir -p "$base"
 /bin/cp "$resources/Presets/Workspace.json" "$base/Workspace.json"
 /bin/cp "$resources/Presets/Arrangements.json" "$base/Arrangements.json"
 /usr/sbin/chown "$user" "$base" "$base/Workspace.json" "$base/Arrangements.json"
fi
