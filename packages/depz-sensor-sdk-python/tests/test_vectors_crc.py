from depz_sensor_sdk.transport import (
    crc8_maxim,
    crc16_modbus,
    crc32_iso_hdlc,
    crc16_ccitt_false,
)

ALGOS = {
    "crc8_maxim": crc8_maxim,
    "crc16_modbus": crc16_modbus,
    "crc32_iso_hdlc": crc32_iso_hdlc,
    "crc16_ccitt_false": crc16_ccitt_false,
}


def test_crc_vectors(vectors):
    data = vectors("crc.json")
    assert data["schema"].startswith("depz.vectors.crc/")
    for case in data["cases"]:
        raw = bytes.fromhex(case["input"])
        for name, fn in ALGOS.items():
            assert fn(raw) == case[name], f"{case['name']}: {name}"
