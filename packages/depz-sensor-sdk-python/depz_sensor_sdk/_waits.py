"""Timed waits: the longest wait the lock primitives take."""

from __future__ import annotations

import threading


def wait_limit(timeout: float) -> float:
    """`timeout` as the lock waits take it: `math.inf`, or anything else past
    `threading.TIMEOUT_MAX`, waits instead of an OverflowError (bug hunt
    2026-11-03 JAVA-10)."""
    return min(timeout, threading.TIMEOUT_MAX)
