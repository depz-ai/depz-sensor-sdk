#!/usr/bin/env bash
# Line coverage of the C SDK (src/) under the CTest suite: the library and the
# runners built with gcc --coverage in their own tree, build-coverage/ (the
# normal build/ is untouched), gcovr over the .gcda files, then the eight
# least-covered files and the total. Usage: scripts/coverage.sh [--all] [--min PCT] [-j N]
#   --all      list every file, not only the least-covered eight
#   --min PCT  the total line-coverage floor (default: FLOOR below; 0 = never fail)
#   -j N       build / ctest parallelism (default 2: the machines this runs on
#              run out of memory above that)
# The floor is a ratchet: the total measured the day it was set, rounded down to
# a whole per cent. Raise it when the tests grow; never lower it to make a change pass.
# Reports: build-coverage/coverage.txt (every file), coverage.xml (Cobertura).
# Needs gcovr (`uv tool install gcovr` or `pip install --user gcovr`) and gcc;
# with clang set CC=clang and GCOV="llvm-cov gcov".
set -euo pipefail

FLOOR=89

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export PATH="$HOME/.local/bin:$PATH"   # where uv tool / pip --user put gcovr
command -v gcovr >/dev/null 2>&1 || {
    echo "coverage: gcovr not found; install it with 'uv tool install gcovr' or 'pip install --user gcovr'" >&2
    exit 2
}

summary_args=()
jobs=2
while [ $# -gt 0 ]; do
    case "$1" in
        --all) summary_args+=(--all); shift ;;
        --min) summary_args+=(--min "$2"); shift 2 ;;
        -j) jobs=$2; shift 2 ;;
        *) echo "usage: scripts/coverage.sh [--all] [--min PCT] [-j N]" >&2; exit 2 ;;
    esac
done

build="$HERE/build-coverage"
# -fprofile-update=atomic: the live layer's reader threads bump the same counters as the
# test thread; plain increments race and gcovr then refuses a negative count.
cmake -S "$HERE" -B "$build" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS="--coverage -fprofile-update=atomic" \
    -DDEPZ_SENSOR_SDK_C_BUILD_EXAMPLES=OFF
find "$build" -name '*.gcda' -delete   # counts of the previous run
cmake --build "$build" -j "$jobs"
ctest --test-dir "$build" -j "$jobs" --output-on-failure

# gcov must match the compiler that wrote the .gcno files (gcov-12 for gcc-12:
# the unversioned gcov may be another major on the same machine).
if [ -z "${GCOV:-}" ]; then
    cc="$(sed -n 's/^CMAKE_C_COMPILER:FILEPATH=//p' "$build/CMakeCache.txt")"
    if "$cc" --version 2>/dev/null | head -n 1 | grep -qi clang; then
        GCOV="llvm-cov gcov"
    else
        GCOV="gcov-$("$cc" -dumpversion | cut -d. -f1)"
        command -v "$GCOV" >/dev/null 2>&1 || GCOV=gcov
    fi
fi

# Only src/ (the tests and the CMake compiler-id probes are instrumented too).
gcovr -r "$HERE" --object-directory "$build" --gcov-executable "$GCOV" \
    --exclude-directories '.*/CMakeFiles/[0-9.]+$' --filter 'src/' \
    --sort uncovered-percent \
    --txt "$build/coverage.txt" --cobertura "$build/coverage.xml" \
    --json-summary "$build/coverage.json"

python3 - "$build/coverage.json" "$FLOOR" "${summary_args[@]}" <<'PY'
import json, sys

path, floor, args = sys.argv[1], float(sys.argv[2]), sys.argv[3:]
show_all = "--all" in args
minimum = float(args[args.index("--min") + 1]) if "--min" in args else floor

report = json.load(open(path))
rows = sorted(((f["line_percent"], f["line_covered"], f["line_total"], f["filename"])
               for f in report["files"] if f["line_total"]), key=lambda r: (r[0], r[3]))
# The serial-port and port-enumeration files run only with a port / on their OS.
needs_port = ("src/io/serial_", "src/io/ports_")
if not show_all:
    rows = [r for r in rows if not r[3].startswith(needs_port)][:8]
    print("Least-covered files (lines; src/io/serial_* and ports_* need a serial port, skipped):")
else:
    print("Line coverage per file:")
for pct, c, t, n in rows:
    print(f"  {pct:6.1f} %  {c:5d}/{t:<5d} {n}")
cov, tot = report["line_covered"], report["line_total"]
total_pct = cov / tot * 100 if tot else 0.0
print(f"Total: {total_pct:.1f} % lines ({cov}/{tot}) over {len(report['files'])} files")
if total_pct < minimum:
    print(f"coverage: total {total_pct:.1f} % is below the {minimum:g} % floor", file=sys.stderr)
    sys.exit(1)
PY
