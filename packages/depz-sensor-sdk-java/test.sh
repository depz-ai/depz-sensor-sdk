#!/usr/bin/env bash
# Build, then run the golden-vector harness against contracts/vectors.
# Exits nonzero if any vector case fails.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VECTORS="${1:-$HERE/../../contracts/vectors}"

"$HERE/build.sh"

echo
java -cp "$HERE/out" ai.depz.sensor.test.RunVectors "$VECTORS"
