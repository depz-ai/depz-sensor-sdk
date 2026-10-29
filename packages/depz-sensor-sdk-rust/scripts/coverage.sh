#!/usr/bin/env bash
# Line coverage of the Rust SDK (src/) under `cargo test`, with cargo-llvm-cov:
# the eight least-covered files and the total. Usage: scripts/coverage.sh [--all] [--min PCT] [-j N]
#   --all      list every file, not only the least-covered eight
#   --min PCT  the total line-coverage floor (default: FLOOR below; 0 = never fail)
#   -j N       build parallelism (default 2: the machines this runs on run out
#              of memory above that)
# The floor is a ratchet: the total measured the day it was set, rounded down to
# a whole per cent. Raise it when the tests grow; never lower it to make a change pass.
# Reports: target/llvm-cov/coverage.json (summary), lcov.info (for CI viewers).
# Needs cargo-llvm-cov (`cargo install cargo-llvm-cov --locked`) and the
# llvm-tools-preview rustup component.
set -euo pipefail

FLOOR=91

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export PATH="$HOME/.cargo/bin:$PATH"
cargo llvm-cov --version >/dev/null 2>&1 || {
    echo "coverage: cargo-llvm-cov not found; install it with 'cargo install cargo-llvm-cov --locked'" >&2
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

cd "$HERE"
out="$HERE/target/llvm-cov"
mkdir -p "$out"
cargo llvm-cov clean --workspace
cargo llvm-cov --no-report -j "$jobs"
cargo llvm-cov report --lcov --output-path "$out/lcov.info"
cargo llvm-cov report --json --summary-only --output-path "$out/coverage.json"

python3 - "$out/coverage.json" "$HERE" "$FLOOR" "${summary_args[@]}" <<'PY'
import json, os, sys

path, root, floor, args = sys.argv[1], sys.argv[2], float(sys.argv[3]), sys.argv[4:]
show_all = "--all" in args
minimum = float(args[args.index("--min") + 1]) if "--min" in args else floor

files = json.load(open(path))["data"][0]["files"]
rows = []
for f in files:
    name = os.path.relpath(f["filename"], root).replace(os.sep, "/")   # Windows: src\x.rs
    if not name.startswith("src/"):
        continue
    lines = f["summary"]["lines"]
    if lines["count"]:
        rows.append((lines["covered"] / lines["count"] * 100, lines["covered"], lines["count"], name))
rows.sort(key=lambda r: (r[0], r[3]))
# The serial-port link opens a real port: it runs only with a board attached.
needs_port = ("src/devices/serial.rs",)
if not show_all:
    rows_shown = [r for r in rows if r[3] not in needs_port][:8]
    print("Least-covered files (lines; src/devices/serial.rs needs a serial port, skipped):")
else:
    rows_shown = rows
    print("Line coverage per file:")
for pct, c, t, n in rows_shown:
    print(f"  {pct:6.1f} %  {c:5d}/{t:<5d} {n}")
cov, tot = sum(r[1] for r in rows), sum(r[2] for r in rows)
total_pct = cov / tot * 100 if tot else 0.0
print(f"Total: {total_pct:.1f} % lines ({cov}/{tot}) over {len(rows)} files")
if total_pct < minimum:
    print(f"coverage: total {total_pct:.1f} % is below the {minimum:g} % floor", file=sys.stderr)
    sys.exit(1)
PY
