import pytest

from depz_sensor_sdk.protocol.bootloader import FwDepzError, FwDepzImage


def test_fwdepz_vectors(vectors):
    for case in vectors("fwdepz.json")["cases"]:
        blob = bytes.fromhex(case["file"])
        if "error" in case:
            with pytest.raises(FwDepzError):
                FwDepzImage.parse(blob)
            continue
        img = FwDepzImage.parse(blob)
        expect = case["expect"]
        assert img.load_addr == expect["load_addr"]
        assert img.fw_size == expect["fw_size"]
        assert img.fw_crc32 == expect["fw_crc32"]
        assert img.cur_sec == expect["cur_sec"]
        assert img.tot_sec == expect["tot_sec"]
        assert img.payload_crc_ok == expect["payload_crc_ok"]
