#!/usr/bin/env bash
# Compile the DEPZ Java SDK (main + test harness) with plain javac.
# No Maven/Gradle; Java 17+ (tested on the installed JDK 21). The one runtime
# dependency, jSerialComm (serial ports), is fetched once into lib/ and checked
# against a pinned SHA-256; Maven builds take it from pom.xml.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT="$HERE/out"
# shellcheck source=deps.sh
source "$HERE/deps.sh"
fetch_deps

rm -rf "$OUT"
mkdir -p "$OUT"

mapfile -t SOURCES < <(find "$HERE/src/main/java" "$HERE/src/test/java" -name '*.java')

echo "javac -> $OUT (${#SOURCES[@]} source files)"
javac --release 17 -cp "$DEPS_CP" -d "$OUT" "${SOURCES[@]}"
# Classpath resources (sensor firmware images), as pom.xml lists them: src/main/resources, and the
# VL53L8 / L5 / L7 blobs straight from the Python SDK's vl53l8/data.
cp -r "$HERE/src/main/resources/." "$OUT/"
mkdir -p "$OUT/ai/depz/sensor/sensors/vl53l8/data"
cp -r "$HERE/../depz-sensor-sdk-python/depz_sensor_sdk/vl53l8/data/." "$OUT/ai/depz/sensor/sensors/vl53l8/data/"
echo "build ok"
