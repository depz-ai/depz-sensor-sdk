import json
import os
from pathlib import Path

import pytest

VECTORS_DIR = Path(__file__).resolve().parents[3] / "contracts" / "vectors"


@pytest.fixture(scope="session")
def vectors():
    def load(name: str) -> dict:
        return json.loads((VECTORS_DIR / name).read_text())

    return load


def seen(case) -> None:
    """Report a golden-vector case ID as asserted (scripts/check_vector_coverage.py).

    Call right AFTER the case's assertions. `case` is a case object (its "id")
    or an ID string. No-op unless $DEPZ_VECTOR_SEEN names the output file; one
    O_APPEND write per line keeps parallel test processes from interleaving.
    """
    path = os.environ.get("DEPZ_VECTOR_SEEN")
    if not path:
        return
    case_id = case["id"] if isinstance(case, dict) else case
    fd = os.open(path, os.O_WRONLY | os.O_APPEND | os.O_CREAT, 0o644)
    try:
        os.write(fd, (case_id + "\n").encode())
    finally:
        os.close(fd)


@pytest.fixture(name="seen", scope="session")
def seen_fixture():
    return seen
