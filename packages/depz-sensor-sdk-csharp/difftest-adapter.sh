#!/usr/bin/env bash
# Differential-testing adapter (contracts/16_DIFFTEST.md), started by
# scripts/difftest.py: the test assembly run with --difftest. Builds it first
# (incremental; output to stderr — stdout is the JSON Lines channel).
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [ -d "$HOME/.dotnet" ] && ! command -v dotnet >/dev/null 2>&1; then
    export DOTNET_ROOT="$HOME/.dotnet" PATH="$HOME/.dotnet:$PATH"
fi
dotnet build "$HERE/tests/Depz.Sensor.Tests" -maxcpucount:2 --nologo -v q -clp:ErrorsOnly >&2
exec dotnet "$HERE/tests/Depz.Sensor.Tests/bin/Debug/net8.0/Depz.Sensor.Tests.dll" --difftest
