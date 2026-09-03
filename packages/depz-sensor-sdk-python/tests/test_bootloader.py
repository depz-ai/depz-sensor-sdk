"""Bootloader client against an in-process fake bootloader (contract 06)."""

import struct
import threading

import pytest

from depz_sensor_sdk.bootloader import BootloaderDevice, BootloaderError
from depz_sensor_sdk.errors import DepzError, LinkClosedError, DeviceLostError
from depz_sensor_sdk.protocol.bootloader import FwDepzImage
from depz_sensor_sdk.transport import CrcType, Packet, PacketParser, build_packet, crc32_iso_hdlc
from depz_sensor_sdk.transport.link import LoopbackLink

APP_START = 0x08006800
PAGE_SIZE = 2048
APP_SIZE = 128 * 1024 - 0x6800


class FakeBootloader:
    """Speaks the bootloader opcode space; flash content stored in a dict."""

    def __init__(self, *, fail_first_write: bool = False):
        self.link, self._side = LoopbackLink.pair()
        self.flash: dict[int, bytes] = {}
        self.erased = False
        self.booted = threading.Event()
        self.fail_first_write = fail_first_write
        self._writes_seen = 0
        self._tx_seq = 0
        self._parser = PacketParser()
        threading.Thread(target=self._run, daemon=True).start()

    def close(self):
        self.link.close()

    def _send(self, rpt: int, payload: bytes) -> None:
        try:
            self._side.write(build_packet(rpt, payload, self._tx_seq, CrcType.NONE))
        except LinkClosedError:
            return
        self._tx_seq = (self._tx_seq + 1) & 0xFF

    def _run(self):
        while True:
            try:
                data = self._side.read(timeout=1.0)
            except (LinkClosedError, DeviceLostError):
                return
            for ev in self._parser.feed(data):
                if isinstance(ev, Packet):
                    self._handle(ev)

    def _app_bytes(self) -> bytes:
        if not self.flash:
            return b""
        end = max(self.flash) + len(self.flash[max(self.flash)])
        blob = bytearray(b"\xff" * (end - APP_START))
        for addr, chunk in self.flash.items():
            blob[addr - APP_START : addr - APP_START + len(chunk)] = chunk
        return bytes(blob)

    def _handle(self, pkt: Packet) -> None:
        cmd = pkt.cmd
        if cmd == 0x04:
            self._send(0x81, bytes([cmd]) + b"BOOTDEPZ_v1.1\x00")
        elif cmd == 0x05:
            self._send(0x81, bytes([cmd]) + b"SN0042\x00")
        elif cmd == 0x03:
            self._send(0x81, bytes([cmd]) + b"DEPZ Test\x00")
        elif cmd == 0x08:
            self._send(0x89, struct.pack("<BHHII", cmd, PAGE_SIZE, 0, APP_START, APP_SIZE))
        elif cmd == 0x09:
            self.flash.clear()
            self.erased = True
            self._send(0x80, bytes([cmd, 0x00]))
        elif cmd == 0x0A:
            addr, size = struct.unpack_from("<IH", pkt.payload)
            chunk = pkt.payload[6 : 6 + size]
            self._writes_seen += 1
            if self.fail_first_write and self._writes_seen == 1:
                # corrupt CRC reply once to exercise the retry path
                self._send(0x8A, bytes([cmd, 0x00]) + struct.pack("<I", 0xDEADBEEF))
                return
            if not (APP_START <= addr < APP_START + APP_SIZE):
                self._send(0x80, bytes([cmd, 0x03]))  # ERR_ADDR
                return
            self.flash[addr] = chunk
            self._send(0x8A, bytes([cmd, 0x00]) + struct.pack("<I", crc32_iso_hdlc(chunk)))
        elif cmd == 0x0C:
            crc = crc32_iso_hdlc(self._app_bytes())
            self._send(0x8C, bytes([cmd, 0x00]) + struct.pack("<I", crc))
        elif cmd == 0x01:
            self.booted.set()
        else:
            self._send(0x80, bytes([cmd, 0x01]))


@pytest.fixture()
def image():
    payload = bytes((i * 7) & 0xFF for i in range(5000))  # 3 pages
    return FwDepzImage.parse(FwDepzImage.build(APP_START, payload))


def test_flash_flow(image):
    fake = FakeBootloader()
    bl = BootloaderDevice(fake.link, timeout=2.0)
    try:
        assert bl.get_firmware_name() == "BOOTDEPZ_v1.1"
        info = bl.get_flash_info()
        assert (info.page_size, info.app_start) == (PAGE_SIZE, APP_START)
        steps = []
        bl.flash(image, progress=lambda f, m: steps.append((f, m)))
        assert fake.erased
        assert fake._app_bytes() == image.payload
        assert steps[-1][0] == 1.0
        bl.boot_application()
        assert fake.booted.wait(1.0)
    finally:
        bl.close()
        fake.close()


def test_flash_retries_bad_page_crc(image):
    fake = FakeBootloader(fail_first_write=True)
    bl = BootloaderDevice(fake.link, timeout=2.0)
    try:
        bl.flash(image)  # first write answers bogus CRC; retry must succeed
        assert fake._app_bytes() == image.payload
        assert fake._writes_seen == 4  # 3 pages + 1 retry
    finally:
        bl.close()
        fake.close()


def test_flash_rejects_wrong_load_addr(image):
    fake = FakeBootloader()
    bl = BootloaderDevice(fake.link, timeout=2.0)
    wrong = FwDepzImage.parse(FwDepzImage.build(0x08000000, image.payload))
    try:
        with pytest.raises(DepzError, match="load_addr"):
            bl.flash(wrong)
    finally:
        bl.close()
        fake.close()
