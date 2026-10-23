#!/usr/bin/env python3
"""Copy the VL53L8 / L5 / L7 sensor firmware and configuration blobs from the
Python SDK (packages/depz-sensor-sdk-python/depz_sensor_sdk/vl53l8/data, the
single source of truth) into src/vl53l8/data, where the driver takes them with
include_bytes!. The crate is published to crates.io and may only carry files
inside its own directory, hence the copy. Stdlib only.

Run from packages/depz-sensor-sdk-rust:  python3 scripts/sync_vl53l8_data.py
     --check: fail when the in-crate copies differ from the Python source
              (a file changed, missing, or one the source no longer has).
"""
import shutil
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
SRC = HERE.parent / "depz-sensor-sdk-python" / "depz_sensor_sdk" / "vl53l8" / "data"
DST = HERE / "src" / "vl53l8" / "data"
SETS = ["cx", "ch", "l7cx", "l7ch"]


def source_files() -> dict:
    files = {}
    for s in SETS:
        for f in sorted((SRC / s).glob("*.bin")):
            files[f"{s}/{f.name}"] = f
    if not files:
        raise SystemExit(f"no blobs under {SRC}")
    return files


def main() -> int:
    want = source_files()
    have = {f"{p.parent.name}/{p.name}": p for p in DST.glob("*/*.bin")} if DST.exists() else {}
    if "--check" in sys.argv:
        stale = [k for k, p in want.items() if k not in have or have[k].read_bytes() != p.read_bytes()]
        extra = [k for k in have if k not in want]
        if stale or extra:
            for k in stale:
                print(f"stale or missing: src/vl53l8/data/{k}")
            for k in extra:
                print(f"not in the Python SDK: src/vl53l8/data/{k}")
            print("run scripts/sync_vl53l8_data.py")
            return 1
        print(f"src/vl53l8/data up to date ({len(want)} blobs)")
        return 0
    for k in have:
        if k not in want:
            have[k].unlink()
    for k, p in want.items():
        out = DST / k
        out.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(p, out)
    print(f"copied {len(want)} blobs into {DST}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
