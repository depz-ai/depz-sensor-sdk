from depz_sensor_sdk.protocol import parse_software_name, strip_device_string


def test_identity_vectors(vectors):
    for case in vectors("identity.json")["cases"]:
        raw = bytes.fromhex(case["raw"])
        ident = parse_software_name(strip_device_string(raw))
        expect = case["expect"]
        assert ident.mode == expect["mode"], case["name"]
        actual_sensor = ident.sensor_type.value if ident.sensor_type else None
        assert actual_sensor == expect["sensor_type"], case["name"]
        assert ident.software_name == expect["software_name"], case["name"]
        assert ident.version == expect["version"], case["name"]
