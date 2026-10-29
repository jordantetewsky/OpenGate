#!/bin/bash
# SPDX-License-Identifier: MIT
set -eu
scripts="$(cd "$(dirname "$0")" && pwd)"
if [[ -z "${OPENGATE_TEST_ROOT:-}" ]]; then
 if /usr/bin/pgrep -x Resolve >/dev/null; then echo 'Save and quit Resolve before recovery.'; exit 1; else code=$?; [[ "$code" == 1 ]] || exit 1; fi
 if [[ "$EUID" -ne 0 ]]; then exec /usr/bin/sudo /bin/bash "$0"; fi
fi
source "$scripts/transaction.sh"
transaction="$scripts"
[[ -f "$transaction/present.txt" && -f "$transaction/paths.txt" ]]
restore_checkpoint
if [[ -f "$product/install.lock/transaction" ]] && [[ "$(cat "$product/install.lock/transaction")" == "$transaction" ]]; then
 /bin/rm "$product/install.lock/transaction"; /bin/rmdir "$product/install.lock"
fi
echo 'Previous OpenGate files restored. User presets and Resolve data were not changed.'
