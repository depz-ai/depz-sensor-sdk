#!/usr/bin/env bash
# Compile the DEPZ Java SDK with plain javac: the SDK, the examples (examples/) and the
# tests when the tree has them.
# No Maven/Gradle; Java 17+ (tested on the installed JDK 21). The one runtime
# dependency, jSerialComm (serial ports), and the JSpecify nullness annotations
# are fetched once into lib/ and checked against pinned SHA-256s; Maven builds
# take them from pom.xml. The SDK is the module `ai.depz.sensor`
# (src/main/java/module-info.java), so javac takes them from the module path;
# the examples and the tests compile into the same module and run on the
# classpath, where the descriptor is inert.
#
# javac runs with Error Prone (Google's bug-pattern checker, a javac plugin
# fetched the same way) and -Werror: any finding fails the build. Off, with the
# reason: ArrayRecordComponent (the decoded frames are records over arrays,
# copied per frame otherwise), MutablePublicArray (the generated register and
# tuning tables are public constant arrays, as in the other SDKs),
# StatementSwitchToExpressionSwitch (style; the ports keep the C switches).
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT="$HERE/out"
# shellcheck source=deps.sh
source "$HERE/deps.sh"
fetch_deps
fetch_errorprone

rm -rf "$OUT"
mkdir -p "$OUT"

SOURCE_DIRS=("$HERE/src/main/java" "$HERE/examples")
[ -d "$HERE/src/test/java" ] && SOURCE_DIRS+=("$HERE/src/test/java")   # only when the tests are present
SOURCES=()   # no mapfile: macOS ships bash 3.2
while IFS= read -r source; do SOURCES+=("$source"); done < <(find "${SOURCE_DIRS[@]}" -name '*.java')

echo "javac -> $OUT (${#SOURCES[@]} source files)"
# Error Prone reaches into javac's internals: open them to the plugin.
JAVAC_OPENS=()
for pkg in api code comp file main model parser processing tree util; do
    JAVAC_OPENS+=("-J--add-exports=jdk.compiler/com.sun.tools.javac.$pkg=ALL-UNNAMED")
done
JAVAC_OPENS+=("-J--add-opens=jdk.compiler/com.sun.tools.javac.code=ALL-UNNAMED"
              "-J--add-opens=jdk.compiler/com.sun.tools.javac.comp=ALL-UNNAMED")
javac "${JAVAC_OPENS[@]}" --release 17 -Werror \
    -XDcompilePolicy=simple -XDaddTypeAnnotationsToSymbol=true --should-stop=ifError=FLOW \
    -processorpath "$ERRORPRONE_PATH" \
    "-Xplugin:ErrorProne -Xep:ArrayRecordComponent:OFF -Xep:MutablePublicArray:OFF -Xep:StatementSwitchToExpressionSwitch:OFF" \
    --module-path "$DEPS_CP" -d "$OUT" "${SOURCES[@]}"
# Classpath resources (sensor firmware images), as pom.xml lists them: src/main/resources, and the
# VL53L8 / L5 / L7 blobs straight from the Python SDK's vl53l8/data.
cp -r "$HERE/src/main/resources/." "$OUT/"
mkdir -p "$OUT/ai/depz/sensor/sensors/vl53l8/data"
cp -r "$HERE/../depz-sensor-sdk-python/depz_sensor_sdk/vl53l8/data/." "$OUT/ai/depz/sensor/sensors/vl53l8/data/"
echo "build ok"
