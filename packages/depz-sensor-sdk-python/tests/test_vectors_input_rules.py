"""`input_rules` groups (contract 07 §3): which inputs the SDK rejects and that
trailing payload bytes are ignored. Each case is `{op, in, expect}` in the
differential-test format (contract 16); it runs through the same SDK calls as
tests/difftest_adapter.py."""

import json

import pytest
from difftest_adapter import handle

FILES = [
    "framing_encode.json",
    "usb_ids.json",
    "common_commands.json",
    "sr04.json",
    "bno055.json",
    "bno086_shtp.json",
    "tmf8829.json",
    "vl53l4.json",
    "vl53l7.json",
    "vl53lx.json",
    "vl53l8.json",
    "vl53l8_advanced.json",
    "depzrec.json",
    "depzdata.json",
]


@pytest.mark.parametrize("fname", FILES)
def test_input_rules(vectors, seen, fname):
    cases = vectors(fname)["input_rules"]
    assert cases, fname
    for c in cases:
        got = handle({"id": 0, "op": c["op"], "in": c["in"]})
        if "error" in c["expect"]:
            assert got.get("error") == "invalid", (c["id"], got)
            # a rejection is a ValueError-style error naming the field, never a crash
            assert got["detail"].split(":")[0] not in ("IndexError", "ZeroDivisionError", "error", "TypeError"), (
                c["id"],
                got,
            )
        else:
            assert "out" in got, (c["id"], got)
            assert json.loads(json.dumps(got["out"])) == c["expect"], c["id"]
        seen(c)
