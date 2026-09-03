#!/usr/bin/env bash
# Compile the DEPZ Java SDK (main + vector-test harness) with plain javac.
# No Maven/Gradle; Java 17+ (tested on the installed JDK 21).
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT="$HERE/out"
rm -rf "$OUT"
mkdir -p "$OUT"

mapfile -t SOURCES < <(find "$HERE/src/main/java" "$HERE/src/test/java" -name '*.java')

echo "javac -> $OUT (${#SOURCES[@]} source files)"
javac --release 17 -d "$OUT" "${SOURCES[@]}"
echo "build ok"
