# Sourced by build.sh / test.sh / difftest-adapter.sh: the jars the SDK needs
# outside the JDK, pinned by version and SHA-256 (the same versions as pom.xml).
JSERIALCOMM_VERSION=2.11.4
JSERIALCOMM_SHA256=b5f7ec40d622864b92dc679159cdbaa3030d37884f0af2deef45b56c16bda7a6

DEPS_DIR="$HERE/lib"
JSERIALCOMM_JAR="$DEPS_DIR/jSerialComm-$JSERIALCOMM_VERSION.jar"
DEPS_CP="$JSERIALCOMM_JAR"

sha256_of() {
    if command -v sha256sum >/dev/null 2>&1; then sha256sum "$1" | cut -d' ' -f1
    else shasum -a 256 "$1" | cut -d' ' -f1; fi
}

fetch_deps() {
    if [ -f "$JSERIALCOMM_JAR" ]; then
        return
    fi
    mkdir -p "$DEPS_DIR"
    local url="https://repo1.maven.org/maven2/com/fazecast/jSerialComm/$JSERIALCOMM_VERSION/jSerialComm-$JSERIALCOMM_VERSION.jar"
    echo "fetching $url" >&2
    curl -fsSL -o "$JSERIALCOMM_JAR.part" "$url"
    local got
    got="$(sha256_of "$JSERIALCOMM_JAR.part")"
    if [ "$got" != "$JSERIALCOMM_SHA256" ]; then
        rm -f "$JSERIALCOMM_JAR.part"
        echo "jSerialComm $JSERIALCOMM_VERSION: SHA-256 $got, expected $JSERIALCOMM_SHA256" >&2
        exit 1
    fi
    mv "$JSERIALCOMM_JAR.part" "$JSERIALCOMM_JAR"
}
