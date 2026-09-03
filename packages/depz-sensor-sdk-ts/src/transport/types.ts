/**
 * Byte-transport abstraction (contracts/07_SDK_FACADE.md §1 "link").
 * WebSerial (browser), serialport (Node), loopback (tests) and .depzrec
 * replay all implement this. The SDK core touches nothing else.
 */

export interface SerialTransportInfo {
  usbVendorId?: number;
  usbProductId?: number;
  path?: string;
  serialNumber?: string;
}

export interface SerialTransport {
  open(opts?: { baudRate?: number }): Promise<void>;
  write(data: Uint8Array): Promise<void>;
  /** Raw chunks as they arrive; ends on close/disconnect. */
  readable(): AsyncIterableIterator<Uint8Array>;
  close(): Promise<void>;
  readonly info: SerialTransportInfo;
  onDisconnect(cb: () => void): () => void;
}
