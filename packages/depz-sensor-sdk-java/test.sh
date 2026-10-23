#!/usr/bin/env bash
# Build, then run the golden-vector harness against contracts/vectors.
# Exits nonzero if any vector case fails.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VECTORS="${1:-$HERE/../../contracts/vectors}"

"$HERE/build.sh"
# shellcheck source=deps.sh
source "$HERE/deps.sh"

echo
java -cp "$HERE/out:$DEPS_CP" ai.depz.sensor.test.RunVectors "$VECTORS"
echo
java -cp "$HERE/out:$DEPS_CP" ai.depz.sensor.test.RunDeviceTests "$VECTORS"
