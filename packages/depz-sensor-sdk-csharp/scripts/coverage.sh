#!/usr/bin/env bash
# Line coverage of Depz.Sensor under the xunit suite (coverlet.collector, Cobertura XML),
# summarised per class without any extra tool: the eight least-covered classes and the
# total. Usage: scripts/coverage.sh [--all] [--min PCT] [extra `dotnet test` args]
#   --all      list every class, not only the least-covered eight
#   --min PCT  the total line-coverage floor (default 94; 0 = never fail)
# The raw report stays at tests/Depz.Sensor.Tests/TestResults/coverage/coverage.cobertura.xml.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
if [ -d "$HOME/.dotnet" ] && ! command -v dotnet >/dev/null 2>&1; then
    export DOTNET_ROOT="$HOME/.dotnet" PATH="$HOME/.dotnet:$PATH"
fi

summary_args=()
test_args=()
while [ $# -gt 0 ]; do
    case "$1" in
        --all) summary_args+=(--all); shift ;;
        --min) summary_args+=(--min "$2"); shift 2 ;;
        *) test_args+=("$1"); shift ;;
    esac
done

out="$HERE/tests/Depz.Sensor.Tests/TestResults/coverage"
rm -rf "$out"
dotnet test "$HERE/tests/Depz.Sensor.Tests" -maxcpucount:2 --nologo \
    --collect:"XPlat Code Coverage" --results-directory "$out" \
    -- DataCollectionRunSettings.DataCollectors.DataCollector.Configuration.Format=cobertura \
       DataCollectionRunSettings.DataCollectors.DataCollector.Configuration.Include='[Depz.Sensor]*' \
    "${test_args[@]}"

# coverlet nests the file under a run GUID; move it to a stable path.
report="$(find "$out" -name coverage.cobertura.xml | head -n 1)"
[ -n "$report" ] || { echo "coverage: no coverage.cobertura.xml under $out" >&2; exit 1; }
mv "$report" "$out/coverage.cobertura.xml"
find "$out" -mindepth 1 -type d -empty -delete

python3 - "$out/coverage.cobertura.xml" "${summary_args[@]}" <<'PY'
import sys, xml.etree.ElementTree as ET

path, args = sys.argv[1], sys.argv[2:]
show_all = "--all" in args
minimum = float(args[args.index("--min") + 1]) if "--min" in args else 94.0

# One class in the XML per nested type / compiler-generated closure: fold them into
# the outer type (the file a reader opens) and count lines, not <class> elements.
# Only the class-level <lines> block: coverlet repeats every line under its <method> too.
per_type = {}
for cls in ET.parse(path).getroot().iter("class"):
    name = cls.get("name").split("/")[0].split("`")[0]
    covered, total = per_type.get(name, (0, 0))
    block = cls.find("lines")
    for line in (block if block is not None else ()):
        total += 1
        covered += int(line.get("hits")) > 0
    per_type[name] = (covered, total)

rows = sorted(((c / t * 100, c, t, n) for n, (c, t) in per_type.items() if t), key=lambda r: (r[0], r[3]))
if not show_all:
    rows = [r for r in rows if not r[3].endswith(("SerialPorts", "SerialPortLink"))][:8]
    print("Least-covered classes (lines; SerialPorts / SerialPortLink need a port, skipped):")
else:
    print("Line coverage per class:")
for pct, c, t, n in rows:
    print(f"  {pct:6.1f} %  {c:5d}/{t:<5d} {n}")
cov = sum(c for c, _ in per_type.values())
tot = sum(t for _, t in per_type.values())
total_pct = cov / tot * 100 if tot else 0.0
print(f"Total: {total_pct:.1f} % lines ({cov}/{tot}) over {len(per_type)} classes")
if total_pct < minimum:
    print(f"coverage: total {total_pct:.1f} % is below the {minimum:g} % floor", file=sys.stderr)
    sys.exit(1)
PY
