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
import { InvalidStateError } from "../errors.js";
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
  /** Callbacks that threw on the read pump (contract 07 §3). */
  callbackErrors: number;
  /** Reports whose handler threw: that packet dropped (contract 07 §3). */
  reportErrors: number;
}

/** A host-observed gap in the device→host seq (contract 01 §5 step 5). */
export interface SeqGap {
  expectedSeq: number;
  receivedSeq: number;
  /** (received − expected) mod 256. */
  missed: number;
}

/**
 * @internal The packet pump under `DepzDevice`: owns the read loop, the TX
 * sequence counter and the link statistics (`DepzDevice.stats`). Not part of
 * the public API.
 */
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
    callbackErrors: 0,
    reportErrors: 0,
  };

  /** CRC type of the frames this link sends; may change between requests. */
  txCrcType: CrcType;
  private readonly transport: SerialTransport;
  private readonly parser = new PacketParser();
  private txSeq = 0;
  private lastRxSeq: number | null = null;
  private started = false;
  private closeFired = false;
  private packetCbs: Array<(pkt: PacketEvent) => void> = [];
  private parserCbs: Array<(ev: ParserEvent) => void> = [];
  private seqGapCbs: Array<(gap: SeqGap) => void> = [];
  private closeCbs: Array<(cause?: unknown) => void> = [];
  private unsubDisconnect: (() => void) | null = null;

  constructor(transport: SerialTransport, opts?: { txCrcType?: CrcType }) {
    this.transport = transport;
    this.txCrcType = opts?.txCrcType ?? CrcType.None;
  }

  /** True once `start()` ran (the read pump is, or was, running). */
  get isStarted(): boolean {
    return this.started;
  }

  /**
   * Begin the read pump. Must be called exactly once, after the transport
   * is open. Ends (and fires `onClose`) when the transport's readable
   * iterator finishes or the transport reports a disconnect. A pump that
   * fails (the transport's read, or anything on the pump context) closes
   * the transport and fires `onClose` with that cause — never an unhandled
   * rejection.
   */
  start(): void {
    if (this.started) throw new InvalidStateError("DepzLink already started");
    this.started = true;
    this.unsubDisconnect = this.transport.onDisconnect(() => this.fireClose());
    void this.pump();
  }

  private async pump(): Promise<void> {
    try {
      for await (const chunk of this.transport.readable()) {
        // Closed (by this side, or lost) while the chunk was on its way:
        // nothing of it reaches a subscriber — the Python reader stops at
        // `_closing` the same way.
        if (this.closeFired) break;
        this.stats.rxBytes += chunk.length;
        for (const ev of this.parser.feed(chunk)) {
          if (this.closeFired) break;
          this.handleParserEvent(ev);
        }
        this.stats.headerErrors = this.parser.headerErrors;
      }
    } catch (err) {
      this.fireClose(err);
      await this.transport.close().catch(() => {});
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
    // The first packet after open has nothing to be compared with. The gap
    // is reported before that packet goes to the parser/packet subscribers.
    if (this.lastRxSeq !== null) {
      const expectedSeq = (this.lastRxSeq + 1) & 0xff;
      if (seq !== expectedSeq) {
        this.stats.seqGaps += 1;
        const gap: SeqGap = { expectedSeq, receivedSeq: seq, missed: (seq - expectedSeq) & 0xff };
        for (const cb of [...this.seqGapCbs]) cb(gap);
      }
    }
    this.lastRxSeq = seq;
  }

  /**
   * Frame one packet with the next TX seq (wraps at 0xFF), for `sendFrame`.
   * An argument framing refuses (contract 07 §3: a payload over 16383
   * bytes, a cmd that is no u8) is a RangeError thrown here, before the seq
   * is consumed — nothing of it is the transport's.
   */
  frame(cmd: number, payload: Uint8Array = new Uint8Array(0)): Uint8Array {
    const frame = buildPacket(cmd, payload, this.txSeq, this.txCrcType);
    this.txSeq = (this.txSeq + 1) & 0xff;
    return frame;
  }

  /** Write a frame from `frame()`; what rejects here is the transport's write. */
  async sendFrame(frame: Uint8Array): Promise<void> {
    await this.transport.write(frame);
    this.stats.txPackets += 1;
    this.stats.txBytes += frame.length;
  }

  /** `frame()` + `sendFrame()`. */
  async send(cmd: number, payload: Uint8Array = new Uint8Array(0)): Promise<void> {
    await this.sendFrame(this.frame(cmd, payload));
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

  /** Subscribe to host-observed seq gaps. Returns an unsubscribe function. */
  onSeqGap(cb: (gap: SeqGap) => void): () => void {
    this.seqGapCbs.push(cb);
    return () => {
      this.seqGapCbs = this.seqGapCbs.filter((c) => c !== cb);
    };
  }

  /**
   * Subscribe to link teardown (fires once; `cause` is set when the read pump
   * failed). Returns an unsubscribe.
   */
  onClose(cb: (cause?: unknown) => void): () => void {
    this.closeCbs.push(cb);
    return () => {
      this.closeCbs = this.closeCbs.filter((c) => c !== cb);
    };
  }

  private fireClose(cause?: unknown): void {
    if (this.closeFired) return;
    this.closeFired = true;
    this.unsubDisconnect?.();
    // On the pump's own exit path: a throwing subscriber must not turn the
    // teardown into an unhandled rejection, nor keep the others from running.
    for (const cb of [...this.closeCbs]) {
      try {
        cb(cause);
      } catch (err) {
        console.error("depz-sensor-sdk: a link close subscriber threw", err);
      }
    }
  }

  /**
   * Close the transport and fire `onClose`. A transport whose `close()`
   * throws still ends the link — the subscribers are told, the pump stops
   * delivering — and the error is the caller's (the Python reference's
   * `close()` raises it after the reader has failed what was pending).
   */
  async close(): Promise<void> {
    try {
      await this.transport.close();
    } finally {
      this.fireClose();
    }
  }
}
