#!/usr/bin/env bash
# Differential-testing adapter (contracts/16_DIFFTEST.md), started by
# scripts/difftest.py. Rebuilds out/ first when any source is newer than the
# compiled adapter (build output goes to stderr: stdout is the JSON Lines channel).
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
STAMP="$HERE/out/ai/depz/sensor/test/DifftestAdapter.class"

if [ ! -f "$STAMP" ] || [ -n "$(find "$HERE/src" -newer "$STAMP" -type f -print -quit)" ]; then
    "$HERE/build.sh" >&2
fi
# shellcheck source=deps.sh
source "$HERE/deps.sh"
exec java -cp "$HERE/out:$DEPS_CP" ai.depz.sensor.test.DifftestAdapter
