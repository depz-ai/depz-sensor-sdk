"""CNH decode parity against the golden vector captured from a live VL53L8CH.

Same vector the C/C++/Java/Rust/C# SDKs consume — keeps all seven bindings
byte-exact on the Compact-Network-Histogram decode path.
"""

import json
import struct
from pathlib import Path

from depz_sensor_sdk import CnhConfig
from depz_sensor_sdk.vl53l8 import cnh

_VECTOR = Path(__file__).resolve().parents[3] / "contracts/vectors/vl53l8_cnh.json"


def test_cnh_decode_matches_golden_vector():
    v = json.loads(_VECTOR.read_text())
    c = v["config"]
    cfg = CnhConfig()
    cfg.init_config(c["start_bin"], c["num_bins"], c["sub_sample"])
    cfg.create_agg_map(c["resolution"], *c["agg_map"])

    raw = bytes.fromhex(v["cnh_raw"])
    decoded = cnh.decode(cfg, raw)
    exp = v["expected"]

    assert struct.unpack_from("<I", raw, 8)[0] == exp["ref_residual_word"]
    assert len(decoded["aggregates"]) == exp["nb_aggregates"]
    for got, want in zip(decoded["aggregates"], exp["aggregates"]):
        assert got["hist_raw"] == want["hist_raw"]
        assert got["hist_scaler"] == want["hist_scaler"]
