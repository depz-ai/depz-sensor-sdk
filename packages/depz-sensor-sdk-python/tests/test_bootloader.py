"""Bootloader client against an in-process fake bootloader (contract 06)."""

import struct
import threading
import time
from types import SimpleNamespace

import pytest
from serial.tools import list_ports

from depz_sensor_sdk import bootloader as bootloader_mod
from depz_sensor_sdk import device as device_mod
from depz_sensor_sdk.bootloader import (
    BootloaderDevice,
    BootloaderError,
    find_bootloader_port,
    ApplicationNotBackError,
    update_firmware,
)
from depz_sensor_sdk.errors import (
    DepzError,
    DepzTimeoutError,
    DeviceLostError,
    InvalidReplyError,
    LinkClosedError,
    StatusError,
)
from depz_sensor_sdk.protocol.bootloader import BlCmd, BlRpt, BlStatus, FwDepzImage, reply_report
from depz_sensor_sdk.transport import CrcType, Packet, PacketParser, build_packet, crc32_iso_hdlc
from depz_sensor_sdk.transport.link import LoopbackLink
from depz_sensor_sdk.usb_ids import DEPZ_USB_VID, DEV_USB_PID, DEV_USB_VID, PID_SR04

from fake_device import FakeSr04

APP_START = 0x08006800
PAGE_SIZE = 2048
APP_SIZE = 128 * 1024 - 0x6800


class FakeBootloader:
    """Speaks the bootloader opcode space; flash content stored in a dict.

    Knobs model misbehaving devices: `silent` drops the reply to those
    opcodes, `write_status`/`verify_status` answer the typed report with a
    non-ACK status, `verify_crc` overrides the streamed application CRC."""

    def __init__(
        self,
        *,
        fail_first_write: bool = False,
        always_bad_page_crc: bool = False,
        drop_first_write: bool = False,
        nack_first_write: bool = False,
        serial: str = "SN0042",
        firmware_name: str = "BOOTDEPZ_v1.1",
        app_size: int = APP_SIZE,
        page_size: int = PAGE_SIZE,
        silent: frozenset[int] = frozenset(),
        write_status: int = BlStatus.ACK,
        verify_status: int = BlStatus.ACK,
        verify_crc: int | None = None,
        erase_status: int = BlStatus.ACK,
        on_boot=None,
    ):
        self.link, self._side = LoopbackLink.pair()
        self.flash: dict[int, bytes] = {}
        self.erased = False
        self.booted = threading.Event()
        self.fail_first_write = fail_first_write
        self.always_bad_page_crc = always_bad_page_crc
        self.drop_first_write = drop_first_write
        self.nack_first_write = nack_first_write
        self.serial = serial
        self.firmware_name = firmware_name
        self.app_size = app_size
        self.page_size = page_size
        self.silent = silent
        self.write_status = write_status
        self.verify_status = verify_status
        self.verify_crc = verify_crc
        self.erase_status = erase_status
        self.on_boot = on_boot
        self.seen: list[int] = []
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
        self.seen.append(cmd)
        if cmd in self.silent:
            return
        if cmd == 0x04:
            self._send(0x81, bytes([cmd]) + self.firmware_name.encode() + b"\x00")
        elif cmd == 0x05:
            self._send(0x81, bytes([cmd]) + self.serial.encode() + b"\x00")
        elif cmd == 0x03:
            self._send(0x81, bytes([cmd]) + b"DEPZ Test\x00")
        elif cmd == 0x08:
            self._send(0x89, struct.pack("<BHHII", cmd, self.page_size, 0, APP_START, self.app_size))
        elif cmd == 0x09:
            if self.erase_status != BlStatus.ACK:
                self._send(0x80, bytes([cmd, self.erase_status]))
                return
            self.flash.clear()
            self.erased = True
            self._send(0x80, bytes([cmd, 0x00]))
        elif cmd == 0x0A:
            addr, size = struct.unpack_from("<IH", pkt.payload)
            chunk = pkt.payload[6 : 6 + size]
            self._writes_seen += 1
            if self.drop_first_write and self._writes_seen == 1:
                return  # no reply at all: the host must time out and retry
            if self.nack_first_write and self._writes_seen == 1:
                self._send(0x80, bytes([cmd, 0x06]))  # ERR_FLASH as the firmware sends it
                return
            if self.always_bad_page_crc or (self.fail_first_write and self._writes_seen == 1):
                # corrupt CRC reply to exercise the retry path
                self._send(0x8A, bytes([cmd, 0x00]) + struct.pack("<I", 0xDEADBEEF))
                return
            if self.write_status != BlStatus.ACK:
                self._send(0x8A, bytes([cmd, self.write_status]) + struct.pack("<I", 0))
                return
            if not (APP_START <= addr < APP_START + self.app_size):
                self._send(0x80, bytes([cmd, 0x03]))  # ERR_ADDR
                return
            self.flash[addr] = chunk
            self._send(0x8A, bytes([cmd, 0x00]) + struct.pack("<I", crc32_iso_hdlc(chunk)))
        elif cmd == 0x0C:
            if self.verify_status != BlStatus.ACK:
                self._send(0x8C, bytes([cmd, self.verify_status]) + struct.pack("<I", 0))
                return
            crc = crc32_iso_hdlc(self._app_bytes()) if self.verify_crc is None else self.verify_crc
            self._send(0x8C, bytes([cmd, 0x00]) + struct.pack("<I", crc))
        elif cmd == 0x01:
            self.booted.set()
            if self.on_boot:
                self.on_boot()
        else:
            self._send(0x80, bytes([cmd, 0x01]))


@pytest.fixture()
def image():
    payload = bytes((i * 7) & 0xFF for i in range(5000))  # 3 pages
    return FwDepzImage.parse(FwDepzImage.build(APP_START, payload))


@pytest.fixture()
def fake_bl():
    """Factory: build a FakeBootloader with knobs, close every one at teardown."""
    fakes: list[FakeBootloader] = []

    def make(**kw):
        fake = FakeBootloader(**kw)
        fakes.append(fake)
        return fake

    yield make
    for fake in fakes:
        fake.close()


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


# ── errors and lifecycle ─────────────────────────────────────────────────────


def test_bootloader_error_names_known_status_and_hexes_unknown():
    err = BootloaderError(BlCmd.WRITE_PAGE, BlStatus.ERR_FLASH)
    assert (err.cmd, err.status) == (BlCmd.WRITE_PAGE, BlStatus.ERR_FLASH)
    assert str(err) == "bootloader cmd 0x0A failed: ERR_FLASH"
    assert isinstance(err, DepzError)
    assert str(BootloaderError(BlCmd.ERASE_APP, 0x42)).endswith("failed: 0x42")


def test_context_manager_closes_link(fake_bl):
    fake = fake_bl()
    with BootloaderDevice(fake.link) as bl:
        assert bl.port == "<link>"
        assert bl.get_device_name() == "DEPZ Test"
        assert bl.get_serial_number() == "SN0042"
    assert fake.link.closed


def test_port_string_opens_serial_link(monkeypatch, fake_bl):
    fake = fake_bl()
    opened = []

    def open_link(port):
        opened.append(port)
        return fake.link

    monkeypatch.setattr(bootloader_mod, "SerialLink", open_link)
    with BootloaderDevice("/dev/ttyFAKE0", timeout=1.0) as bl:
        assert bl.port == "/dev/ttyFAKE0"
        assert bl.get_firmware_name() == "BOOTDEPZ_v1.1"
    assert opened == ["/dev/ttyFAKE0"]
    assert fake.link.closed


# ── transact ─────────────────────────────────────────────────────────────────


def test_transact_times_out_when_device_is_silent(fake_bl):
    fake = fake_bl(silent=frozenset({BlCmd.GET_FLASH_INFO}))
    with BootloaderDevice(fake.link, timeout=0.3) as bl:
        with pytest.raises(DepzTimeoutError) as ei:
            bl.get_flash_info()
        assert (ei.value.cmd, ei.value.timeout) == (BlCmd.GET_FLASH_INFO, 0.3)
        # the explicit per-call timeout wins over the device default
        with pytest.raises(DepzTimeoutError) as ei:
            bl.transact(BlCmd.GET_FLASH_INFO, timeout=0.1)
        assert ei.value.timeout == 0.1
        assert bl.get_firmware_name() == "BOOTDEPZ_v1.1"  # still usable afterwards


def test_transact_skips_unrelated_short_and_junk_frames(fake_bl):
    fake = fake_bl(silent=frozenset({BlCmd.GET_FLASH_INFO}))
    with BootloaderDevice(fake.link, timeout=1.0) as bl:
        # line noise, a reply echoing another opcode, a FLASH_INFO too short
        # for min_len — none of them completes the request
        fake._side.write(b"\x00\xff\x7e junk")
        fake._send(0x81, bytes([BlCmd.GET_DEVICE_NAME]) + b"other\x00")
        fake._send(0x89, bytes([BlCmd.GET_FLASH_INFO, 0, 8]))
        fake._send(0x89, struct.pack("<BHHII", BlCmd.GET_FLASH_INFO, PAGE_SIZE, 0, APP_START, 64))
        info = bl.get_flash_info()
        assert (info.page_size, info.app_start, info.app_size) == (PAGE_SIZE, APP_START, 64)


def test_transact_finds_the_reply_among_frames_of_one_read(fake_bl):
    """Over USB CDC several small frames arrive in one read. An ACK ahead of a
    typed reply (contract 06 §1) or a stray report must not swallow the frames
    after it — once only the first frame of a chunk was kept and the reply
    was lost to a timeout."""
    fake = fake_bl(silent=frozenset({BlCmd.GET_FLASH_INFO, BlCmd.GET_SERIAL}))
    with BootloaderDevice(fake.link, timeout=0.5) as bl:
        fake._side.write(
            build_packet(BlRpt.STATUS, bytes([BlCmd.GET_FLASH_INFO, BlStatus.ACK]), 0)
            + build_packet(BlRpt.FLASH_INFO,
                           struct.pack("<BHHII", BlCmd.GET_FLASH_INFO, PAGE_SIZE, 0, APP_START, 64), 1)
        )
        assert bl.get_flash_info().page_size == PAGE_SIZE
        fake._side.write(
            build_packet(BlRpt.STRING, bytes([BlCmd.GET_DEVICE_NAME]) + b"DEPZ\x00", 2)
            + build_packet(BlRpt.STRING, bytes([BlCmd.GET_SERIAL]) + b"SN1\x00", 3)
        )
        assert bl.get_serial_number() == "SN1"


def test_transact_raises_on_non_ack_status(fake_bl):
    fake = fake_bl()
    with BootloaderDevice(fake.link, timeout=1.0) as bl:
        # READ_PAGE is not implemented by the fake → RPT_STATUS(cmd, ERROR)
        with pytest.raises(BootloaderError) as ei:
            bl.transact(BlCmd.READ_PAGE, struct.pack("<IH", APP_START, 16))
        assert (ei.value.cmd, ei.value.status) == (BlCmd.READ_PAGE, BlStatus.ERROR)


def test_transact_requires_the_typed_report_of_the_command(fake_bl):
    # ERRATA A15: a report of another kind echoing the opcode in payload[0] is
    # not the reply, however long it is — only RPT_FLASH_INFO answers
    # GET_FLASH_INFO (contract 06 §1).
    fake = fake_bl(silent=frozenset({BlCmd.GET_FLASH_INFO}))
    with BootloaderDevice(fake.link, timeout=1.0) as bl:
        body = struct.pack("<BHHII", BlCmd.GET_FLASH_INFO, 4096, 0, 0x08010000, 32)
        fake._send(BlRpt.STRING, body)  # 13 B, would have passed min_len
        fake._send(BlRpt.WRITE_PAGE, body)
        fake._send(BlRpt.STATUS, bytes([BlCmd.GET_FLASH_INFO, BlStatus.ACK]))  # ACK is no answer
        real = struct.pack("<BHHII", BlCmd.GET_FLASH_INFO, PAGE_SIZE, 0, APP_START, 64)
        fake._send(BlRpt.FLASH_INFO, real)
        info = bl.get_flash_info()
        assert (info.page_size, info.app_start, info.app_size) == (PAGE_SIZE, APP_START, 64)
        # and a typed-reply command never completes on impostor reports alone
        fake._send(BlRpt.STRING, body)
        fake._send(BlRpt.STATUS, bytes([BlCmd.GET_FLASH_INFO, BlStatus.ACK]))
        with pytest.raises(DepzTimeoutError):
            bl.transact(BlCmd.GET_FLASH_INFO, timeout=0.2, min_len=13)


def test_transact_status_reply_command_ignores_typed_reports(fake_bl):
    # The other direction: ERASE_APP answers RPT_STATUS; an RPT_WRITE_PAGE
    # echoing 0x09 with an ACK byte is not it.
    fake = fake_bl(silent=frozenset({BlCmd.ERASE_APP}))
    with BootloaderDevice(fake.link, timeout=1.0) as bl:
        fake._send(BlRpt.WRITE_PAGE, bytes([BlCmd.ERASE_APP, BlStatus.ACK]) + bytes(4))
        with pytest.raises(DepzTimeoutError):
            bl.transact(BlCmd.ERASE_APP, timeout=0.2)
        fake._send(BlRpt.WRITE_PAGE, bytes([BlCmd.ERASE_APP, BlStatus.ACK]) + bytes(4))
        fake._send(BlRpt.STATUS, bytes([BlCmd.ERASE_APP, BlStatus.ACK]))
        bl.erase_app()  # the real one completes it


def test_transact_keeps_accepting_a_status_nack_for_a_typed_reply_command(fake_bl):
    # The firmware's send_nack answers every failure with RPT_STATUS; the
    # report check must not turn a nack into a timeout.
    fake = fake_bl(silent=frozenset({BlCmd.VERIFY_APP_CRC}))
    with BootloaderDevice(fake.link, timeout=5.0) as bl:
        fake._send(BlRpt.STATUS, bytes([BlCmd.VERIFY_APP_CRC, BlStatus.ERR_FLASH]))
        t0 = time.monotonic()
        with pytest.raises(BootloaderError) as ei:
            bl.verify_app_crc()
        assert time.monotonic() - t0 < 1.0
        assert (ei.value.cmd, ei.value.status) == (BlCmd.VERIFY_APP_CRC, BlStatus.ERR_FLASH)


def test_reply_report_table_and_commands_without_a_reply(fake_bl):
    assert reply_report(BlCmd.GET_FLASH_INFO) == BlRpt.FLASH_INFO
    assert reply_report(BlCmd.WRITE_PAGE) == BlRpt.WRITE_PAGE
    assert reply_report(BlCmd.VERIFY_APP_CRC) == BlRpt.VERIFY_APP_CRC
    assert reply_report(BlCmd.ERASE_APP) == BlRpt.STATUS
    assert reply_report(BlCmd.GET_SERIAL) == BlRpt.STRING
    for cmd in (BlCmd.BOOT_APPLICATION, 0x0D, 0x80):
        with pytest.raises(ValueError, match="no reply"):
            reply_report(cmd)
    fake = fake_bl()
    with BootloaderDevice(fake.link, timeout=1.0) as bl:
        with pytest.raises(ValueError):
            bl.transact(BlCmd.BOOT_APPLICATION)
    assert fake.seen == []  # refused before anything reached the wire


# ── flashing primitives ──────────────────────────────────────────────────────


def test_erase_app_reports_flash_error(fake_bl):
    fake = fake_bl(erase_status=BlStatus.ERR_FLASH)
    with BootloaderDevice(fake.link, timeout=1.0) as bl:
        with pytest.raises(BootloaderError, match="0x09 failed: ERR_FLASH"):
            bl.erase_app()
    assert not fake.erased


def test_write_page_out_of_app_area_is_err_addr(fake_bl):
    # The firmware nacks with the 2-byte RPT_STATUS, shorter than the typed
    # RPT_WRITE_PAGE: it must surface at once, not after the reply timeout.
    fake = fake_bl()
    with BootloaderDevice(fake.link, timeout=5.0) as bl:
        t0 = time.monotonic()
        with pytest.raises(BootloaderError) as ei:
            bl.write_page(APP_START - PAGE_SIZE, b"\x00" * 16)
        assert time.monotonic() - t0 < 1.0
        assert (ei.value.cmd, ei.value.status) == (BlCmd.WRITE_PAGE, BlStatus.ERR_ADDR)
        assert fake.flash == {}


def test_flash_retries_a_page_the_firmware_nacked(image, fake_bl):
    fake = fake_bl(nack_first_write=True)
    with BootloaderDevice(fake.link, timeout=5.0) as bl:
        t0 = time.monotonic()
        bl.flash(image)
        assert time.monotonic() - t0 < 2.0
    assert fake._app_bytes() == image.payload
    assert fake._writes_seen == 4  # 3 pages + the nacked first attempt


def test_write_page_non_ack_in_typed_report(fake_bl):
    fake = fake_bl(write_status=BlStatus.ERR_CRC_PKT)
    with BootloaderDevice(fake.link, timeout=1.0) as bl:
        with pytest.raises(BootloaderError) as ei:
            bl.write_page(APP_START, b"\x00" * 16)
        assert (ei.value.cmd, ei.value.status) == (BlCmd.WRITE_PAGE, BlStatus.ERR_CRC_PKT)


def test_write_page_returns_device_crc(fake_bl):
    fake = fake_bl()
    chunk = bytes(range(256))
    with BootloaderDevice(fake.link, timeout=1.0) as bl:
        assert bl.write_page(APP_START + PAGE_SIZE, chunk) == crc32_iso_hdlc(chunk)
    assert fake.flash == {APP_START + PAGE_SIZE: chunk}


def test_verify_app_crc_non_ack(fake_bl):
    fake = fake_bl(verify_status=BlStatus.ERR_FLASH)
    with BootloaderDevice(fake.link, timeout=1.0) as bl:
        with pytest.raises(BootloaderError) as ei:
            bl.verify_app_crc()
        assert (ei.value.cmd, ei.value.status) == (BlCmd.VERIFY_APP_CRC, BlStatus.ERR_FLASH)


# ── full flow: failure modes ─────────────────────────────────────────────────


def test_flash_rejects_image_larger_than_app_area(image, fake_bl):
    fake = fake_bl(app_size=4096)  # image is 5000 B
    with BootloaderDevice(fake.link, timeout=1.0) as bl:
        with pytest.raises(DepzError, match="exceeds app area"):
            bl.flash(image)
    assert not fake.erased  # rejected before touching the flash


@pytest.mark.parametrize(
    "knobs",
    [{"page_size": 0}, {"app_size": APP_SIZE + 1}],
    ids=["page_size_zero", "app_size_not_page_multiple"],
)
def test_flash_rejects_an_impossible_flash_info_before_erasing(image, fake_bl, knobs):
    """Contract 07 §3: a reply value the client cannot use is invalid_reply,
    never a bare ZeroDivisionError — and the app area is left untouched."""
    fake = fake_bl(**knobs)
    with BootloaderDevice(fake.link, timeout=2.0) as bl:
        with pytest.raises(InvalidReplyError) as ei:
            bl.flash(image)
    assert ei.value.cmd == BlCmd.GET_FLASH_INFO
    assert BlCmd.ERASE_APP not in fake.seen and not fake.erased


def test_flash_gives_up_after_page_retries_on_bad_crc(image, fake_bl):
    fake = fake_bl(always_bad_page_crc=True)
    with BootloaderDevice(fake.link, timeout=1.0) as bl:
        with pytest.raises(DepzError, match="CRC mismatch after 3 tries"):
            bl.flash(image)
    assert fake._writes_seen == 3


def test_flash_retries_on_write_error_then_raises(image, fake_bl):
    fake = fake_bl(write_status=BlStatus.ERR_FLASH)
    with BootloaderDevice(fake.link, timeout=1.0) as bl:
        with pytest.raises(BootloaderError) as ei:
            bl.flash(image, page_retries=2)
        assert ei.value.status == BlStatus.ERR_FLASH
    assert fake._writes_seen == 2


def test_flash_retries_a_page_the_device_never_answered(image, fake_bl):
    fake = fake_bl(drop_first_write=True)
    with BootloaderDevice(fake.link, timeout=0.3) as bl:
        bl.flash(image)
    assert fake._app_bytes() == image.payload
    assert fake._writes_seen == 4  # 3 pages + the timed-out first attempt


def test_flash_detects_application_crc_mismatch(image, fake_bl):
    fake = fake_bl(verify_crc=0x12345678)
    steps = []
    with BootloaderDevice(fake.link, timeout=1.0) as bl:
        with pytest.raises(DepzError, match="app CRC mismatch"):
            bl.flash(image, progress=lambda f, m: steps.append((f, m)))
    assert fake._app_bytes() == image.payload  # pages were all written
    assert steps[-1] == (0.92, "verifying application CRC")


def test_flash_progress_sequence(image, fake_bl):
    fake = fake_bl()
    steps = []
    with BootloaderDevice(fake.link, timeout=1.0) as bl:
        bl.flash(image, progress=lambda f, m: steps.append((f, m)))
    fracs = [f for f, _ in steps]
    assert fracs == sorted(fracs) and fracs[0] == 0.0 and fracs[-1] == 1.0
    assert [m for _, m in steps] == [
        "erasing flash",
        "page 1/3",
        "page 2/3",
        "page 3/3",
        "verifying application CRC",
        "flash complete",
    ]


# ── port re-discovery and the app-mode → bootloader → flash flow ─────────────


class FakeApp(FakeSr04):
    """App-mode firmware whose BOOTLOADER command "reboots" the port: the
    virtual bus then opens a FakeBootloader with the same USB serial."""

    def __init__(
        self,
        bus: "FakeBus",
        port: str,
        *,
        silent: frozenset[int] = frozenset(),
        refuse: frozenset[int] = frozenset(),
    ):
        self.bus, self.port_name, self.silent, self.refuse = bus, port, silent, refuse
        self.rebooted = False
        super().__init__()

    def _handle(self, pkt: Packet) -> None:
        if pkt.cmd in self.silent:
            return
        if pkt.cmd in self.refuse:
            self._status(pkt.cmd, 0x01)  # ERR_UNKNOWN_CMD
            return
        if pkt.cmd == 0x01:
            self._status(pkt.cmd, 0x00)
            self.rebooted = True
            self.bus.ports[self.port_name] = lambda: self.bus.bootloader(serial=self.SERIAL)
            return
        super()._handle(pkt)


class FakeBus:
    """What `list_ports.comports()` and `SerialLink(port)` see: a dict of
    port → factory of a fresh device for every open (a LoopbackLink closes
    both ends, exactly like a real port that is closed and reopened).

    `after_boot(port, *steps)` scripts what a BOOT_APPLICATION does to
    `port`: each later open takes the next step (None = the port is gone,
    else a device factory), the last step repeating."""

    def __init__(self):
        self.ports: dict[str, object] = {}
        # port → (vid, pid, USB iSerial); a port not listed is a dev-default
        # DEPZ unit whose OS gives no iSerial.
        self.usb: dict[str, tuple[int, int, str | None]] = {}
        self._boot_steps: dict[str, list] = {}
        self._scripts: dict[str, list] = {}
        self.bootloaders: list[FakeBootloader] = []
        self.apps: list[FakeApp] = []
        self.opened: list[str] = []

    def bootloader(self, **kw) -> FakeBootloader:
        fake = FakeBootloader(on_boot=self._booted, **kw)
        self.bootloaders.append(fake)
        return fake

    def app(self, port: str, **kw) -> FakeApp:
        fake = FakeApp(self, port, **kw)
        self.apps.append(fake)
        return fake

    def after_boot(self, port: str, *steps) -> None:
        self._boot_steps[port] = list(steps)

    def _booted(self) -> None:
        for port, steps in self._boot_steps.items():
            self._scripts[port] = list(steps)

    def open(self, port: str):
        self.opened.append(port)
        script = self._scripts.get(port)
        if script is not None:
            step = script.pop(0) if len(script) > 1 else script[0]
            if step is None:
                raise DeviceLostError(f"cannot open {port}: no such port")
            return step().link
        factory = self.ports.get(port)
        if factory is None:
            raise DeviceLostError(f"cannot open {port}: busy")
        return factory().link

    def comports(self):
        default = (DEV_USB_VID, DEV_USB_PID, None)
        return [
            SimpleNamespace(device=p, vid=vid, pid=pid, serial_number=serial)
            for p in self.ports
            for vid, pid, serial in [self.usb.get(p, default)]
        ]

    def close(self):
        for fake in self.bootloaders + self.apps:
            fake.close()


@pytest.fixture()
def bus(monkeypatch):
    bus = FakeBus()
    monkeypatch.setattr(list_ports, "comports", bus.comports)
    monkeypatch.setattr(bootloader_mod, "SerialLink", bus.open)
    monkeypatch.setattr(device_mod, "SerialLink", bus.open)
    yield bus
    bus.close()


def test_find_bootloader_port_matches_name_and_serial(bus):
    bus.ports["/dev/ttyACM0"] = None  # cannot be opened (held by someone else)
    bus.ports["/dev/ttyACM1"] = lambda: bus.app("/dev/ttyACM1")  # app mode
    bus.ports["/dev/ttyACM2"] = lambda: bus.bootloader(serial="SN0001")  # other board
    bus.ports["/dev/ttyACM3"] = lambda: bus.bootloader(silent=frozenset({0x04}))  # mute
    bus.ports["/dev/ttyACM4"] = lambda: bus.bootloader(serial="SN0042")

    assert find_bootloader_port("SN0042", timeout=3.0) == "/dev/ttyACM4"
    assert bus.opened == [f"/dev/ttyACM{i}" for i in range(5)]
    # every probed device is released, including the one we settled on
    assert all(f.link.closed for f in bus.bootloaders + bus.apps)
    # the app-mode port was recognised by name alone: no serial query
    assert bus.bootloaders[0].seen == [0x04, 0x05]
    assert bus.bootloaders[1].seen == [0x04]  # mute: timed out on the name


def test_find_bootloader_port_talks_only_to_depz_candidates(bus):
    """Contract 06 §3 / 07 §3: only ports with a DEPZ USB id are candidates,
    and one whose USB iSerial names another board is not opened at all."""
    bus.ports["/dev/ttyUSB0"] = lambda: bus.bootloader(serial="SN0042")  # foreign USB id
    bus.usb["/dev/ttyUSB0"] = (0x2341, 0x0043, "SN0042")
    bus.ports["/dev/ttyACM1"] = lambda: bus.bootloader(serial="SN0001")  # another DEPZ board
    bus.usb["/dev/ttyACM1"] = (DEPZ_USB_VID, PID_SR04, "SN0001")
    bus.ports["/dev/ttyACM2"] = lambda: bus.bootloader(serial="SN0042")  # no iSerial: probed

    assert find_bootloader_port("SN0042", timeout=3.0) == "/dev/ttyACM2"
    assert bus.opened == ["/dev/ttyACM2"]


def test_find_bootloader_port_opens_the_iserial_match_first(bus):
    bus.ports["/dev/ttyACM0"] = lambda: bus.bootloader(serial="SN0042")  # no iSerial
    bus.ports["/dev/ttyACM1"] = lambda: bus.bootloader(serial="SN0042")
    bus.usb["/dev/ttyACM1"] = (DEPZ_USB_VID, PID_SR04, "SN0042")

    assert find_bootloader_port("SN0042", timeout=3.0) == "/dev/ttyACM1"
    assert bus.opened == ["/dev/ttyACM1"]


def test_find_bootloader_port_times_out(bus):
    bus.ports["/dev/ttyACM0"] = lambda: bus.bootloader(serial="SN0001")
    with pytest.raises(DepzTimeoutError) as ei:
        find_bootloader_port("SN0042", timeout=0.5)
    assert ei.value.cmd == BlCmd.GET_FIRMWARE_NAME
    assert len(bus.opened) >= 2  # kept polling until the deadline


def test_update_firmware_from_app_mode(bus, image, tmp_path):
    fw = tmp_path / "app.fwdepz"
    fw.write_bytes(FwDepzImage.build(APP_START, image.payload))
    bus.ports["/dev/ttyACM0"] = lambda: bus.app("/dev/ttyACM0")
    bus.after_boot("/dev/ttyACM0", lambda: bus.app("/dev/ttyACM0"))
    steps = []

    name = update_firmware("/dev/ttyACM0", str(fw), progress=lambda f, m: steps.append((f, m)))

    assert name == FakeSr04.SOFTWARE_NAME
    assert bus.apps[0].rebooted and bus.apps[0].link.closed
    flashed = bus.bootloaders[-1]
    assert flashed.erased and flashed._app_bytes() == image.payload
    assert flashed.booted.wait(1.0) and flashed.link.closed
    assert steps[0] == (0.0, "rebooting into bootloader")
    assert steps[-1] == (1.0, "flash complete")


def test_update_firmware_when_already_in_bootloader(bus, image, tmp_path):
    fw = tmp_path / "app.fwdepz"
    fw.write_bytes(FwDepzImage.build(APP_START, image.payload))
    bus.ports["/dev/ttyACM0"] = lambda: bus.bootloader()
    bus.after_boot("/dev/ttyACM0", lambda: bus.app("/dev/ttyACM0"))
    steps = []

    name = update_firmware("/dev/ttyACM0", str(fw), progress=lambda f, m: steps.append((f, m)))

    # probe, reopen as the bootloader, then the re-probe after the reboot
    assert bus.opened == ["/dev/ttyACM0"] * 3
    assert name == FakeSr04.SOFTWARE_NAME and len(bus.apps) == 1
    flashed = bus.bootloaders[1]
    assert flashed._app_bytes() == image.payload and flashed.booted.wait(1.0)
    assert "rebooting into bootloader" not in [m for _, m in steps]


def test_update_firmware_treats_mute_name_query_as_app_mode(bus, image, tmp_path):
    fw = tmp_path / "app.fwdepz"
    fw.write_bytes(FwDepzImage.build(APP_START, image.payload))
    bus.ports["/dev/ttyACM0"] = lambda: bus.app("/dev/ttyACM0", silent=frozenset({0x04}))
    bus.after_boot("/dev/ttyACM0", lambda: bus.app("/dev/ttyACM0"))

    update_firmware("/dev/ttyACM0", str(fw))

    assert bus.apps[0].rebooted
    assert bus.bootloaders[-1]._app_bytes() == image.payload


def test_update_firmware_rejects_bad_file_before_touching_the_port(bus, tmp_path):
    fw = tmp_path / "bad.fwdepz"
    fw.write_bytes(b"not a firmware")
    bus.ports["/dev/ttyACM0"] = lambda: bus.app("/dev/ttyACM0")
    with pytest.raises(ValueError):
        update_firmware("/dev/ttyACM0", str(fw))
    assert bus.opened == []


def test_update_firmware_waits_past_the_bootloader_for_the_application(bus, image, tmp_path):
    """Contract 06 §3 step 6 (ERRATA A7): after BOOT_APPLICATION the port is
    gone, then back under the same name as the bootloader, then the app."""
    fw = tmp_path / "app.fwdepz"
    fw.write_bytes(FwDepzImage.build(APP_START, image.payload))
    bus.ports["/dev/ttyACM0"] = lambda: bus.bootloader()
    bus.after_boot(
        "/dev/ttyACM0",
        None,
        None,
        lambda: bus.bootloader(),
        lambda: bus.app("/dev/ttyACM0"),
    )

    name = update_firmware("/dev/ttyACM0", str(fw))

    assert name == FakeSr04.SOFTWARE_NAME
    # probe + flash, two misses, the bootloader once, then the application
    assert bus.opened == ["/dev/ttyACM0"] * 6
    assert bus.bootloaders[2].seen == [BlCmd.GET_FIRMWARE_NAME]
    assert bus.bootloaders[2].link.closed
    assert len(bus.apps) == 1 and bus.apps[0].link.closed


@pytest.mark.parametrize(
    "steps, last_seen",
    [
        ((None,), "no port"),
        ((None, "bootloader"), "the bootloader (BOOTDEPZ_v1.1)"),
    ],
    ids=["never_back", "stays_in_bootloader"],
)
def test_update_firmware_times_out_naming_what_it_last_saw(bus, image, tmp_path, steps, last_seen):
    fw = tmp_path / "app.fwdepz"
    fw.write_bytes(FwDepzImage.build(APP_START, image.payload))
    bus.ports["/dev/ttyACM0"] = lambda: bus.bootloader()
    bus.after_boot(
        "/dev/ttyACM0", *[None if s is None else (lambda: bus.bootloader()) for s in steps]
    )

    with pytest.raises(ApplicationNotBackError) as ei:
        update_firmware("/dev/ttyACM0", str(fw), app_timeout=0.5)

    assert isinstance(ei.value, DepzTimeoutError)
    assert ei.value.last_seen == last_seen
    assert f"(last seen: {last_seen})" in str(ei.value)
    assert ei.value.port == "/dev/ttyACM0" and ei.value.timeout == 0.5
    assert len(bus.opened) >= 4  # kept polling until the deadline
    assert bus.bootloaders[1]._app_bytes() == image.payload


@pytest.mark.parametrize(
    "refuse",
    [0x04, 0x05, 0x01],
    ids=["software_name", "serial", "bootloader_cmd"],
)
def test_update_firmware_closes_the_app_connection_when_the_probe_fails(
    bus, image, tmp_path, refuse
):
    """Any failure before the hand-off to the bootloader releases the port:
    a retry in the same process must be able to open it again."""
    fw = tmp_path / "app.fwdepz"
    fw.write_bytes(FwDepzImage.build(APP_START, image.payload))
    bus.ports["/dev/ttyACM0"] = lambda: bus.app("/dev/ttyACM0", refuse=frozenset({refuse}))

    with pytest.raises(StatusError) as ei:
        update_firmware("/dev/ttyACM0", str(fw))

    assert ei.value.cmd == refuse
    assert len(bus.apps) == 1 and bus.apps[0].link.closed
    assert not bus.bootloaders
