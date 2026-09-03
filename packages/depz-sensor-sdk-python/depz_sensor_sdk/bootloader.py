"""Bootloader client: application firmware update over USB (contract 06).

Excluded by design: ROM DFU entry and bootloader self-update.
"""

from __future__ import annotations

import time
from typing import Callable

from .errors import DepzError, DepzTimeoutError, DeviceLostError
from .protocol.bootloader import (
    BlCmd,
    BlRpt,
    BlStatus,
    FlashInfo,
    FwDepzImage,
    pack_write_page,
)
from .protocol.common import strip_device_string
from .transport import CrcType, Packet, PacketParser, build_packet, crc32_iso_hdlc
from .transport.link import Link
from .transport.serial_link import SerialLink

_STATUS_NAMES = {s.value: s.name for s in BlStatus}

ProgressFn = Callable[[float, str], None]


class BootloaderError(DepzError):
    def __init__(self, cmd: int, status: int):
        self.cmd = cmd
        self.status = status
        name = _STATUS_NAMES.get(status, f"0x{status:02X}")
        super().__init__(f"bootloader cmd 0x{cmd:02X} failed: {name}")


class BootloaderDevice:
    """Synchronous client for a device already in bootloader mode.

    The bootloader protocol is strict request/response — no reader thread."""

    def __init__(self, port_or_link: str | Link, *, timeout: float = 5.0):
        if isinstance(port_or_link, str):
            self._link: Link = SerialLink(port_or_link)
            self.port = port_or_link
        else:
            self._link = port_or_link
            self.port = getattr(port_or_link, "port", "<link>")
        self.timeout = timeout
        self._parser = PacketParser()
        self._tx_seq = 0

    def close(self) -> None:
        self._link.close()

    def __enter__(self) -> "BootloaderDevice":
        return self

    def __exit__(self, *exc: object) -> None:
        self.close()

    # ── low level ────────────────────────────────────────────────────────────

    def _send(self, cmd: int, payload: bytes = b"", crc_type: CrcType = CrcType.NONE) -> None:
        self._link.write(build_packet(cmd, payload, self._tx_seq, crc_type))
        self._tx_seq = (self._tx_seq + 1) & 0xFF

    def _recv(self, timeout: float | None = None) -> Packet | None:
        deadline = time.monotonic() + (timeout if timeout is not None else self.timeout)
        while time.monotonic() < deadline:
            data = self._link.read(0.05)
            for ev in self._parser.feed(data):
                if isinstance(ev, Packet):
                    return ev
        return None

    def transact(
        self,
        cmd: int,
        payload: bytes = b"",
        *,
        crc_type: CrcType = CrcType.NONE,
        timeout: float | None = None,
        min_len: int = 2,
    ) -> Packet:
        """Send and wait for the reply echoing `cmd` in payload[0]."""
        self._send(cmd, payload, crc_type)
        deadline = time.monotonic() + (timeout if timeout is not None else self.timeout)
        while True:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise DepzTimeoutError(cmd, timeout if timeout is not None else self.timeout)
            pkt = self._recv(remaining)
            if pkt is None:
                continue
            if pkt.payload and pkt.payload[0] == cmd and len(pkt.payload) >= min_len:
                if pkt.cmd == BlRpt.STATUS and pkt.payload[1] != BlStatus.ACK:
                    raise BootloaderError(cmd, pkt.payload[1])
                return pkt

    # ── info ─────────────────────────────────────────────────────────────────

    def _text(self, cmd: BlCmd) -> str:
        pkt = self.transact(cmd, timeout=2.0)
        return strip_device_string(pkt.payload[1:])

    def get_firmware_name(self) -> str:
        return self._text(BlCmd.GET_FIRMWARE_NAME)

    def get_device_name(self) -> str:
        return self._text(BlCmd.GET_DEVICE_NAME)

    def get_serial_number(self) -> str:
        return self._text(BlCmd.GET_SERIAL)

    def get_flash_info(self) -> FlashInfo:
        pkt = self.transact(BlCmd.GET_FLASH_INFO, min_len=13)
        return FlashInfo.unpack(pkt.payload)

    # ── flashing primitives ──────────────────────────────────────────────────

    def erase_app(self) -> None:
        pkt = self.transact(BlCmd.ERASE_APP, timeout=15.0)
        if pkt.payload[1] != BlStatus.ACK:
            raise BootloaderError(BlCmd.ERASE_APP, pkt.payload[1])

    def write_page(self, addr: int, data: bytes) -> int:
        """Write one page (payload CRC16 framing); returns the device-computed
        page CRC32."""
        pkt = self.transact(
            BlCmd.WRITE_PAGE,
            pack_write_page(addr, data),
            crc_type=CrcType.CRC16,
            min_len=6,
        )
        if pkt.payload[1] != BlStatus.ACK:
            raise BootloaderError(BlCmd.WRITE_PAGE, pkt.payload[1])
        return int.from_bytes(pkt.payload[2:6], "little")

    def verify_app_crc(self) -> int:
        pkt = self.transact(BlCmd.VERIFY_APP_CRC, timeout=10.0, min_len=6)
        if pkt.payload[1] != BlStatus.ACK:
            raise BootloaderError(BlCmd.VERIFY_APP_CRC, pkt.payload[1])
        return int.from_bytes(pkt.payload[2:6], "little")

    def boot_application(self) -> None:
        """Fire-and-forget: the device jumps to the app and re-enumerates."""
        self._send(BlCmd.BOOT_APPLICATION)
        time.sleep(0.2)
        self.close()

    # ── full flow ────────────────────────────────────────────────────────────

    def flash(
        self,
        image: FwDepzImage,
        *,
        progress: ProgressFn | None = None,
        page_retries: int = 3,
    ) -> None:
        """ERASE → WRITE_PAGE loop (per-page CRC32 check, retries) → VERIFY.

        Does not boot the app — call `boot_application()` after."""

        def report(frac: float, msg: str) -> None:
            if progress:
                progress(frac, msg)

        info = self.get_flash_info()
        if image.load_addr != info.app_start:
            raise DepzError(
                f"load_addr mismatch: file=0x{image.load_addr:08X} "
                f"device=0x{info.app_start:08X}"
            )
        if image.fw_size > info.app_size:
            raise DepzError(f"image {image.fw_size} B exceeds app area {info.app_size} B")

        report(0.0, "erasing flash")
        self.erase_app()

        payload = image.payload
        page = info.page_size
        num_pages = (len(payload) + page - 1) // page
        for idx in range(num_pages):
            chunk = payload[idx * page : (idx + 1) * page]
            addr = info.app_start + idx * page
            expected_crc = crc32_iso_hdlc(chunk)
            for attempt in range(1, page_retries + 1):
                try:
                    got = self.write_page(addr, chunk)
                except (BootloaderError, DepzTimeoutError):
                    if attempt == page_retries:
                        raise
                    continue
                if got == expected_crc:
                    break
                if attempt == page_retries:
                    raise DepzError(
                        f"page @0x{addr:08X}: CRC mismatch after {page_retries} tries"
                    )
            report(0.05 + 0.85 * (idx + 1) / num_pages, f"page {idx + 1}/{num_pages}")

        report(0.92, "verifying application CRC")
        dev_crc = self.verify_app_crc()
        if dev_crc != image.fw_crc32:
            raise DepzError(
                f"app CRC mismatch: device=0x{dev_crc:08X} header=0x{image.fw_crc32:08X}"
            )
        report(1.0, "flash complete")


def find_bootloader_port(serial_number: str, *, timeout: float = 10.0) -> str:
    """Poll serial ports until a BOOTDEPZ device with `serial_number` appears
    (after an app→bootloader reboot the USB serial string is preserved)."""
    from serial.tools import list_ports

    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        for p in list_ports.comports():
            try:
                bl = BootloaderDevice(p.device, timeout=0.5)
            except (DeviceLostError, DepzError):
                continue
            try:
                if not bl.get_firmware_name().upper().startswith("BOOTDEPZ"):
                    continue
                if bl.get_serial_number() == serial_number:
                    bl.close()
                    return p.device
            except (DepzTimeoutError, DeviceLostError, DepzError):
                pass
            finally:
                bl.close()
        time.sleep(0.3)
    raise DepzTimeoutError(BlCmd.GET_FIRMWARE_NAME, timeout)


def update_firmware(
    port: str,
    fwdepz_path: str,
    *,
    progress: ProgressFn | None = None,
) -> None:
    """Full app-mode → bootloader → flash → boot-app flow (contract 06 §3)."""
    from .device import DeviceBase

    image = FwDepzImage.load(fwdepz_path)

    dev = DeviceBase(port, timeout=0.5)
    try:
        name = dev.get_software_name()
    except DepzTimeoutError:
        name = ""
    if name.upper().startswith("BOOTDEPZ"):
        # Already in bootloader mode; pyserial can't share the port — reopen.
        dev.close()
        bl = BootloaderDevice(port)
    else:
        serial_number = dev.get_serial_number()
        if progress:
            progress(0.0, "rebooting into bootloader")
        dev.enter_bootloader_mode()  # also closes
        bl_port = find_bootloader_port(serial_number)
        bl = BootloaderDevice(bl_port)

    try:
        bl.flash(image, progress=progress)
        bl.boot_application()
    finally:
        bl.close()
