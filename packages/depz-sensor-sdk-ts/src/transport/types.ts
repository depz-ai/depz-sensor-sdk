/**
 * Byte-transport abstraction (contract 07 §1 "link").
 * WebSerial (browser), serialport (Node), loopback (tests) and .depzrec
 * replay all implement this. The SDK core touches nothing else.
 */

export interface SerialTransportInfo {
  usbVendorId?: number;
  usbProductId?: number;
  path?: string;
  serialNumber?: string;
}

/**
 * How the SDK calls a transport (contract 07 §3, "The host-supplied link") —
 * what a transport given to `openOnTransport` or a device constructor has to
 * cope with. Everything runs on the event loop: there are no threads and no
 * read slices.
 *
 * - `open` is called once, on a transport not yet open; `readable` once, after
 *   it, and that iterator is the only reader for the whole connection.
 * - `write` is called once per frame, a whole frame per call, in frame order
 *   and **without waiting for the previous `write`**: unlike the other SDKs,
 *   this one does not serialize writes, so a transport keeps them in order
 *   itself. It resolves when everything is written and rejects otherwise, and
 *   it must settle within a bounded time: a request waits for its `write`, so
 *   one that never settles never ends, not even at its timeout.
 * - `close` is called by every `DepzDevice.close()` and when the read fails
 *   (then again by the program's own `close()`): it must be idempotent. It
 *   may come during a pending read or `write`, and it must end the `readable`
 *   iterator; the device does not wait for that iterator.
 * - `onDisconnect` is subscribed once, when the read starts.
 * - A loss, unless the program is closing the device: `readable` throwing
 *   (logged with `console.error`, kept as the `cause`; the SDK closes the
 *   transport), `readable` ending, or the `onDisconnect` callback (no cause;
 *   the SDK does not call `close` until the program does). The device closes
 *   and every later request fails with `DeviceLostError`.
 * - A rejected `write` fails only that request, as a `DepzError` as it is and
 *   as `DeviceLostError` otherwise (`LinkClosedError` while closing), the
 *   rejection as its `cause`; the device ends when the read reports the loss.
 * - A rejected `close` reaches the program from its own `close()` (the device
 *   still ends); after a failed read it is dropped.
 */
export interface SerialTransport {
  open(opts?: { baudRate?: number }): Promise<void>;
  /** One whole frame; resolves when all of it is written, within a bounded time. */
  write(data: Uint8Array): Promise<void>;
  /** Raw chunks as they arrive; ends on close/disconnect. */
  readable(): AsyncIterableIterator<Uint8Array>;
  /** Idempotent; ends a pending `readable`. */
  close(): Promise<void>;
  readonly info: SerialTransportInfo;
  onDisconnect(cb: () => void): () => void;
}
