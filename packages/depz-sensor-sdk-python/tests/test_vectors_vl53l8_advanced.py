"""Golden-vector consumer: vl53l8_advanced.json.

Drives the real ULD methods through the fake DCI platform and asserts the
DCI-write transcripts equal the frozen vectors (py↔ts parity).
"""

import struct

from depz_sensor_sdk.vl53l8 import uld

from fake_vl53l8 import make_driver


def test_motion_config_pack_vectors(vectors):
    for case in vectors("vl53l8_advanced.json")["motion"]:
        drv, p = make_driver()
        drv.motion_indicator_init(case["resolution"])
        stored = p.dci[uld.DCI_MOTION_DETECTOR_CFG]
        assert stored.hex() == case["pack"], case["name"]


def test_detection_threshold_pack_vectors(vectors):
    for case in vectors("vl53l8_advanced.json")["thresholds"]:
        drv, p = make_driver()
        drv.set_detection_thresholds(case["thresholds"])
        assert p.dci[uld.DCI_DET_THRESH_START].hex() == case["start_block"], case["name"]
        assert (
            p.dci[uld.DCI_DET_THRESH_VALID_STATUS].hex() == case["valid_status"]
        ), case["name"]


def test_xtalk_margin_vectors(vectors):
    for case in vectors("vl53l8_advanced.json")["xtalk_margin"]:
        drv, p = make_driver()
        drv.set_xtalk_margin(case["kcps"])
        raw = struct.unpack_from("<I", p.dci[uld.DCI_XTALK_CFG], 0)[0]
        assert raw == case["raw"], case["name"]
