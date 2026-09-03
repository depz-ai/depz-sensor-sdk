import json
from pathlib import Path

import pytest

VECTORS_DIR = Path(__file__).resolve().parents[3] / "contracts" / "vectors"


@pytest.fixture(scope="session")
def vectors():
    def load(name: str) -> dict:
        return json.loads((VECTORS_DIR / name).read_text())

    return load
