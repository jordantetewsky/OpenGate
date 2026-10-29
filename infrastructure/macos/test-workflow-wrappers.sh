#!/bin/bash
set -eu
root="$(cd "$(dirname "$0")/../.." && pwd)"
python3 "$root/tests/platforms/macos/test-workflow-command-wrappers.py"
python3 "$root/tests/core/operations/test_audio_channel.py"
