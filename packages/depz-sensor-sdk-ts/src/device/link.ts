/**
 * DepzLink: packet-level pump over a byte transport
 * (contracts/07_SDK_FACADE.md §1 layer 1→2 boundary).
 *
 * Owns the read pump (`transport.readable()` → `PacketParser`), the
 * auto-incrementing TX sequence counter, and the link statistics. Packet
 * routing/correlation lives one layer up in `DepzDevice`.
 */

import {
  buildPacket,
  CrcType,
  PacketParser,
  type PacketEvent,
  type ParserEvent,
} from "../protocol/framing.js";
import type { SerialTransport } from "../transport/types.js";

/** Link statistics (contract 07 §2 `stats`). */
export interface LinkStats {
  rxPackets: number;
  txPackets: number;
  rxBytes: number;
  txBytes: number;
  crcErrors: number;
  headerErrors: number;
  trashBytes: number;
  /** Host-observed gaps in the device→host seq counter (mod 256). */
  seqGaps: number;
}

export class DepzLink {
  readonly stats: LinkStats = {
    rxPackets: 0,
    txPackets: 0,
    rxBytes: 0,
    txBytes: 0,
    crcErrors: 0,
    headerErrors: 0,
    trashBytes: 0,
    seqGaps: 0,
  };

  private readonly transport: SerialTransport;
  private readonly txCrcType: CrcType;
  private readonly parser = new PacketParser();
  private txSeq = 0;
  private lastRxSeq: number | null = null;
  private started = false;
  private closeFired = false;
  private packetCbs: Array<(pkt: PacketEvent) => void> = [];
  private parserCbs: Array<(ev: ParserEvent) => void> = [];
  private closeCbs: Array<() => void> = [];
  private unsubDisconnect: (() => void) | null = null;

  constructor(transport: SerialTransport, opts?: { txCrcType?: CrcType }) {
    this.transport = transport;
    this.txCrcType = opts?.txCrcType ?? CrcType.None;
  }

  /**
   * Begin the read pump. Must be called exactly once, after the transport
   * is open. Ends (and fires `onClose`) when the transport's readable
   * iterator finishes or the transport reports a disconnect.
   */
  start(): void {
    if (this.started) throw new Error("DepzLink already started");
    this.started = true;
    this.unsubDisconnect = this.transport.onDisconnect(() => this.fireClose());
    void this.pump();
  }

  private async pump(): Promise<void> {
    try {
      for await (const chunk of this.transport.readable()) {
        this.stats.rxBytes += chunk.length;
        for (const ev of this.parser.feed(chunk)) this.handleParserEvent(ev);
        this.stats.headerErrors = this.parser.headerErrors;
      }
    } finally {
      this.fireClose();
    }
  }

  private handleParserEvent(ev: ParserEvent): void {
    if (ev.type === "packet") {
      this.stats.rxPackets += 1;
      this.trackSeq(ev.seq);
    } else if (ev.type === "crcError") {
      this.stats.crcErrors += 1;
    } else {
      this.stats.trashBytes += ev.data.length;
    }
    for (const cb of [...this.parserCbs]) cb(ev);
    if (ev.type === "packet") {
      for (const cb of [...this.packetCbs]) cb(ev);
    }
  }

  private trackSeq(seq: number): void {
    if (this.lastRxSeq !== null && seq !== ((this.lastRxSeq + 1) & 0xff)) {
      this.stats.seqGaps += 1;
    }
    this.lastRxSeq = seq;
  }

  /** Frame and write one packet with the next TX seq (wraps at 0xFF). */
  async send(cmd: number, payload: Uint8Array = new Uint8Array(0)): Promise<void> {
    const frame = buildPacket(cmd, payload, this.txSeq, this.txCrcType);
    this.txSeq = (this.txSeq + 1) & 0xff;
    await this.transport.write(frame);
    this.stats.txPackets += 1;
    this.stats.txBytes += frame.length;
  }

  /** Subscribe to decoded packets. Returns an unsubscribe function. */
  onPacket(cb: (pkt: PacketEvent) => void): () => void {
    this.packetCbs.push(cb);
    return () => {
      this.packetCbs = this.packetCbs.filter((c) => c !== cb);
    };
  }

  /**
   * Subscribe to all parser events, including CRC errors and trash
   * (diagnostics; the device layer maps these to `DeviceEvent`s).
   */
  onParserEvent(cb: (ev: ParserEvent) => void): () => void {
    this.parserCbs.push(cb);
    return () => {
      this.parserCbs = this.parserCbs.filter((c) => c !== cb);
    };
  }

  /** Subscribe to link teardown (fires once). Returns an unsubscribe. */
  onClose(cb: () => void): () => void {
    this.closeCbs.push(cb);
    return () => {
      this.closeCbs = this.closeCbs.filter((c) => c !== cb);
    };
  }

  private fireClose(): void {
    if (this.closeFired) return;
    this.closeFired = true;
    this.unsubDisconnect?.();
    for (const cb of [...this.closeCbs]) cb();
  }

  async close(): Promise<void> {
    await this.transport.close();
    this.fireClose();
  }
}
