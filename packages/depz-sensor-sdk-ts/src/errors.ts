/**
 * Exception hierarchy of the DEPZ sensor SDK.
 * Status code names follow contracts/02_COMMON_COMMANDS.md §3.
 */

/** Base class for all SDK errors. */
export class DepzError extends Error {
  constructor(message?: string) {
    super(message);
    this.name = new.target.name;
  }
}

function hex2(n: number): string {
  return n.toString(16).toUpperCase().padStart(2, "0");
}

/** A request got no matching reply within the timeout. */
export class DepzTimeoutError extends DepzError {
  readonly cmd: number;
  readonly timeoutMs: number;

  constructor(cmd: number, timeoutMs: number) {
    super(`no reply to cmd 0x${hex2(cmd)} within ${timeoutMs}ms`);
    this.cmd = cmd;
    this.timeoutMs = timeoutMs;
  }
}

const STATUS_NAMES: Record<number, string> = {
  0x00: "OK",
  0x01: "ERROR",
  0x02: "ERR_INVALID_CMD",
  0x03: "ERR_PAYLOAD_FORMAT",
  0x04: "ERR_INVALID_PARAM",
  0x05: "ERR_PAYLOAD_CRC",
  0x06: "ERR_BUSY",
  0x07: "ERR_CMD_NOT_SUPPORTED",
  0x08: "ERR_NOT_INITIALIZED",
  0x09: "ERR_HARDWARE_FAULT",
};

/** Device answered a request with a non-OK RPT_STATUS. */
export class StatusError extends DepzError {
  readonly cmd: number;
  readonly status: number;
  readonly statusName: string;

  constructor(cmd: number, status: number) {
    const statusName = STATUS_NAMES[status] ?? `0x${hex2(status)}`;
    super(`cmd 0x${hex2(cmd)} failed: ${statusName} (0x${hex2(status)})`);
    this.cmd = cmd;
    this.status = status;
    this.statusName = statusName;
  }
}

/** Device answered ERR_BUSY; the operation may be retried later. */
export class BusyError extends StatusError {
  constructor(cmd: number) {
    super(cmd, 0x06);
  }
}

/**
 * The reply to a request does not decode (contract 07 §3): shorter than the
 * fields its contract defines, or a value the decoder refuses. Only that
 * request fails; the read pump goes on and the device stays usable.
 */
export class InvalidReplyError extends DepzError {
  readonly cmd: number;

  constructor(cmd: number, cause: unknown) {
    super(`reply to cmd 0x${hex2(cmd)} does not decode: ${cause instanceof Error ? cause.message : String(cause)}`);
    this.cmd = cmd;
    this.cause = cause;
  }
}

/** The serial link dropped (unplug, reboot) while in use. */
export class DeviceLostError extends DepzError {}

/** Operation attempted on a closed link/device. */
export class LinkClosedError extends DepzError {}

/**
 * The call is not valid in the current state of the device or object
 * (contract 07 §3): `toHostTimeUs` before `syncTime`, a sensor call before
 * `init()` / `configure()` or while streaming, `add()` after recording
 * started, a closed dataset writer or player, `wait()` without the player
 * thread, a write to a read-only register block, a VL53Lx board name without
 * a product number, a packaged firmware blob of the wrong size.
 */
export class InvalidStateError extends DepzError {}

/**
 * A strict `ReplayTransport` saw the host write bytes the capture does not
 * hold (contract 08): the SDK under test diverged from the recorded protocol.
 */
export class ReplayMismatchError extends DepzError {}

/**
 * Discovery found no DEPZ device matching the request (no candidate port by
 * USB id, an out-of-range index, or a serial that nothing answered to).
 * Mirrors the Python reference `NoDepzDeviceError`.
 */
export class NoDepzDeviceError extends DepzError {}
