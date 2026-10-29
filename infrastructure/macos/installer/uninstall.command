#!/bin/bash
# SPDX-License-Identifier: MIT
set -eu
scripts="$(cd "$(dirname "$0")" && pwd)"
if [[ -z "${OPENGATE_TEST_ROOT:-}" ]]; then
 if /usr/bin/pgrep -x Resolve >/dev/null; then echo 'Save and quit Resolve before uninstalling.'; exit 1; else code=$?; [[ "$code" == 1 ]] || exit 1; fi
 if [[ "$EUID" -ne 0 ]]; then exec /usr/bin/sudo /bin/bash "$0"; fi
fi
source "$scripts/transaction.sh"
new_transaction
if ! mkdir "$product/install.lock" 2>/dev/null; then fail 'Install or recovery is pending; complete recovery before uninstalling'; fi
trap 'rmdir "$product/install.lock" 2>/dev/null || true' EXIT
checkpoint "$scripts/paths.txt"
cp "$scripts/transaction.sh" "$transaction/transaction.sh"
cp "$scripts/rollback.command" "$transaction/Restore Previous OpenGate.command"
chmod 755 "$transaction/Restore Previous OpenGate.command"
while IFS= read -r path; do /bin/rm -rf "$(physical "$path")"; done < "$transaction/paths.txt"
log 'uninstalled; OpenGate settings and native Resolve data preserved'
/bin/rmdir "$product/install.lock"
trap - EXIT
echo 'OpenGate removed. Reopen Resolve for its native UI. Your presets remain available for reinstall.'
echo "Recovery: $transaction/Restore Previous OpenGate.command"
