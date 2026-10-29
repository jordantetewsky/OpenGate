#!/bin/bash
# SPDX-License-Identifier: MIT
# Shared by install, rollback and uninstall. Settings are never a removal target.
set -eu
physical() { printf '%s%s' "${OPENGATE_TEST_ROOT:-}" "$1"; }
product=$(physical '/Library/Application Support/CineWaltz/OpenGate')
mkdir -p "$product/Transactions"
log() { printf '%s %s\n' "$(/bin/date -u +%Y-%m-%dT%H:%M:%SZ)" "$*" >> "$transaction/events.log"; }
fail() { log "failed $*"; echo "OpenGate: $*. Diagnostic record: $transaction/events.log" >&2; exit 1; }
new_transaction() {
 transaction="$product/Transactions/$(/usr/bin/uuidgen)"
 mkdir -m 755 "$transaction"
}
version_relation() {
 local -a installed offered
 local i left right
 IFS=. read -r -a installed <<< "$1"
 IFS=. read -r -a offered <<< "$2"
 for i in 0 1 2; do
  left=$((10#${installed[$i]})); right=$((10#${offered[$i]}))
  if [[ "$right" -gt "$left" ]]; then echo upgrade; return; fi
  if [[ "$right" -lt "$left" ]]; then echo downgrade; return; fi
 done
 echo reinstall
}
validate_path() {
 case "$1" in
  '/Applications/OpenGate Tools/Export OpenGate Support.command'|'/Applications/OpenGate Tools/Uninstall OpenGate.command'|'/Applications/OpenGate.app'|'/Library/Application Support/CineWaltz/OpenGate/Current'|'/Library/Application Support/Blackmagic Design/DaVinci Resolve/IOPlugins/Unbound.dvcp.bundle') ;;
  '/Library/Application Support/Blackmagic Design/DaVinci Resolve/Workflow Integration Plugins/'OpenGate*.py) ;;
  '/Library/Application Support/Blackmagic Design/DaVinci Resolve/IOPlugins/'Unbound-retired-*.backup)
   [[ "${1##*/}" =~ ^Unbound-retired-[0-9]{8}T[0-9]{6}Z\.backup$ ]] || fail 'Invalid retired loader identity' ;;
  *) fail 'Inventory contains a path outside OpenGate managed files' ;;
 esac
 [[ "$1" != *'/../'* ]] || fail 'Invalid package inventory'
}
checkpoint() {
 local inventory=$1 index=0 path target
 cp "$inventory" "$transaction/paths.txt"
 mkdir "$transaction/files"
 : > "$transaction/present.txt"
 while IFS= read -r path; do
  [[ -n "$path" && "$path" == /* && "$path" != *'/../'* ]] || fail 'Invalid package inventory'
  validate_path "$path"
  target=$(physical "$path")
  if [[ -e "$target" || -L "$target" ]]; then
   /usr/bin/ditto "$target" "$transaction/files/$index"
   printf '%s\n' "$index" >> "$transaction/present.txt"
  fi
  index=$((index+1))
 done < "$inventory"
 log 'checkpoint complete'
}
restore_checkpoint() {
 local index=0 path target
 while IFS= read -r path; do
  validate_path "$path"
  target=$(physical "$path")
  # Inventory comes from this package, never from user settings or a wildcard.
  /bin/rm -rf "$target"
  if /usr/bin/grep -qx "$index" "$transaction/present.txt"; then
   /bin/mkdir -p "$(/usr/bin/dirname "$target")"
   /usr/bin/ditto "$transaction/files/$index" "$target"
  fi
  index=$((index+1))
 done < "$transaction/paths.txt"
 log 'rollback complete'
}
