#!/bin/bash
# SPDX-License-Identifier: MIT
# Retired OpenGate loaders must be backed up outside Resolve's plugin search tree.
legacy_loaders() {
 local folder name metadata product_name
 local directory='/Library/Application Support/Blackmagic Design/DaVinci Resolve/IOPlugins'
 for folder in "$(physical "$directory")"/Unbound-retired-*.backup; do
  [[ -d "$folder" ]] || continue
  name=${folder##*/}
  [[ "$name" =~ ^Unbound-retired-[0-9]{8}T[0-9]{6}Z\.backup$ ]] || continue
  metadata="$folder/Contents/Resources/build-manifest.json"
  [[ -f "$metadata" && -f "$folder/Contents/MacOS/Unbound.dvcp" && -f "$folder/Contents/MacOS/UnboundWidgetProbe.dylib" ]] || continue
  product_name=$(/usr/bin/plutil -extract product raw -o - "$metadata" 2>/dev/null) || continue
  [[ "$product_name" == OpenGate ]] || continue
  printf '%s/%s\n' "$directory" "$name"
 done
}
