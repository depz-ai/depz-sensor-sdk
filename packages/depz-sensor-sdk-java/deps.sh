# Sourced by build.sh / test.sh / difftest-adapter.sh: the jars the SDK needs
# outside the JDK, pinned by version and SHA-256 (the same versions as pom.xml),
# and the build-time Error Prone jars build.sh compiles with.
JSERIALCOMM_VERSION=2.11.4
JSERIALCOMM_SHA256=b5f7ec40d622864b92dc679159cdbaa3030d37884f0af2deef45b56c16bda7a6
ERRORPRONE_VERSION=2.50.0
ERRORPRONE_SHA256=8ec037a6d57c0d880ed78c6a67445e5018a17a89b42cb6847ddef9081c504378
# the dataflow library Error Prone $ERRORPRONE_VERSION is built against (its pom's dataflow.version)
DATAFLOW_VERSION=3.41.0-eisop1
DATAFLOW_SHA256=10434fba4e53f55fa9c76904cde414b918932548c9dfc4e2d634ac05ff7a7d10

DEPS_DIR="$HERE/lib"
JSERIALCOMM_JAR="$DEPS_DIR/jSerialComm-$JSERIALCOMM_VERSION.jar"
ERRORPRONE_JAR="$DEPS_DIR/error_prone_core-$ERRORPRONE_VERSION-with-dependencies.jar"
DATAFLOW_JAR="$DEPS_DIR/dataflow-errorprone-$DATAFLOW_VERSION.jar"
DEPS_CP="$JSERIALCOMM_JAR"
ERRORPRONE_PATH="$ERRORPRONE_JAR:$DATAFLOW_JAR"

sha256_of() {
    if command -v sha256sum >/dev/null 2>&1; then sha256sum "$1" | cut -d' ' -f1
    else shasum -a 256 "$1" | cut -d' ' -f1; fi
}

# fetch_jar <file> <Maven Central path> <sha256>: once, checked against the pinned hash
fetch_jar() {
    local jar=$1 path=$2 want=$3
    if [ -f "$jar" ]; then
        return
    fi
    mkdir -p "$DEPS_DIR"
    local url="https://repo1.maven.org/maven2/$path"
    echo "fetching $url" >&2
    curl -fsSL -o "$jar.part" "$url"
    local got
    got="$(sha256_of "$jar.part")"
    if [ "$got" != "$want" ]; then
        rm -f "$jar.part"
        echo "$(basename "$jar"): SHA-256 $got, expected $want" >&2
        exit 1
    fi
    mv "$jar.part" "$jar"
}

fetch_deps() {
    fetch_jar "$JSERIALCOMM_JAR" \
        "com/fazecast/jSerialComm/$JSERIALCOMM_VERSION/jSerialComm-$JSERIALCOMM_VERSION.jar" "$JSERIALCOMM_SHA256"
}

fetch_errorprone() {
    fetch_jar "$ERRORPRONE_JAR" \
        "com/google/errorprone/error_prone_core/$ERRORPRONE_VERSION/$(basename "$ERRORPRONE_JAR")" "$ERRORPRONE_SHA256"
    fetch_jar "$DATAFLOW_JAR" \
        "io/github/eisop/dataflow-errorprone/$DATAFLOW_VERSION/$(basename "$DATAFLOW_JAR")" "$DATAFLOW_SHA256"
}
