/**
 * uld/bare/image — the device state the BareDriver keeps, and how it moves.
 * Mirror of the Python `depz_sensor_sdk.vl53lx.uld.bare.image`.
 *
 * The C driver holds one struct per register block inside
 * `VL53LX_LLDriverData_t` and never reads a configuration register on its own:
 * it edits the struct, then pushes whole blocks. Ranging start is the extreme
 * case — `VL53LX_init_and_start_range()` writes 0x0001..0x0087 in a single
 * 135-byte transfer. Block moves keep the USB round trips where the C driver
 * has them.
 *
 * The blocks are almost contiguous: static_nvm ends at 0x000C and customer_nvm
 * starts at 0x000D. The C driver zeroes the whole buffer first and drops each
 * block at its own offset, so the reserved byte goes out as 0x00;
 * `pushRange()` does the same, for the same bytes on the wire.
 */

import type { BridgePlatform } from "../base.js";
import { BLOCKS, BLOCK_NAMES, RegBlock, type BlockName } from "./regs.js";

/** The blocks `init_and_start_range` pushes, in address order. */
export const RANGE_START_BLOCKS: readonly BlockName[] = [
  "static_nvm_managed",
  "customer_nvm_managed",
  "static_config",
  "general_config",
  "timing_config",
  "dynamic_config",
  "system_control",
];

/**
 * The device's register image, block by block. Blocks are reached as
 * properties under their C name minus the type suffix:
 * `img.static_config.v.dss_config__target_total_rate_mcps`. Editing a field
 * only touches the host copy; `push()` is what reaches the sensor.
 */
export class DeviceImage {
  readonly blocks: Record<BlockName, RegBlock>;

  constructor(readonly p: BridgePlatform) {
    const blocks = {} as Record<BlockName, RegBlock>;
    for (const name of BLOCK_NAMES) blocks[name] = new RegBlock(name);
    this.blocks = blocks;
  }

  get static_nvm_managed(): RegBlock { return this.blocks.static_nvm_managed; }
  get customer_nvm_managed(): RegBlock { return this.blocks.customer_nvm_managed; }
  get static_config(): RegBlock { return this.blocks.static_config; }
  get general_config(): RegBlock { return this.blocks.general_config; }
  get timing_config(): RegBlock { return this.blocks.timing_config; }
  get dynamic_config(): RegBlock { return this.blocks.dynamic_config; }
  get system_control(): RegBlock { return this.blocks.system_control; }
  get system_results(): RegBlock { return this.blocks.system_results; }
  get core_results(): RegBlock { return this.blocks.core_results; }
  get debug_results(): RegBlock { return this.blocks.debug_results; }
  get nvm_copy_data(): RegBlock { return this.blocks.nvm_copy_data; }

  // ── one block ──
  /** Read one block off the sensor into the image. */
  async pull(name: BlockName): Promise<RegBlock> {
    const blk = this.blocks[name];
    return blk.decode(await this.p.rdMulti(blk.base, blk.size));
  }

  /** Write one block from the image to the sensor. */
  async push(name: BlockName): Promise<void> {
    const blk = this.blocks[name];
    if (!BLOCKS[name].writable) throw new Error(`${name} is read-only`);
    await this.p.wrMulti(blk.base, blk.encode());
  }

  // ── a run of blocks, as one transfer ──
  /** [base, total size] covering the named blocks, reserved bytes included. */
  static span(names: readonly BlockName[]): [number, number] {
    const base = Math.min(...names.map((n) => BLOCKS[n].base));
    const end = Math.max(...names.map((n) => BLOCKS[n].base + BLOCKS[n].size));
    return [base, end - base];
  }

  async pullRange(names: readonly BlockName[]): Promise<void> {
    const [base, size] = DeviceImage.span(names);
    const raw = await this.p.rdMulti(base, size);
    for (const name of names) {
      const blk = this.blocks[name];
      const off = blk.base - base;
      blk.decode(raw.subarray(off, off + blk.size));
    }
  }

  async pushRange(names: readonly BlockName[]): Promise<void> {
    const [base, size] = DeviceImage.span(names);
    const buf = new Uint8Array(size);
    for (const name of names) {
      if (!BLOCKS[name].writable) throw new Error(`${name} is read-only`);
      const blk = this.blocks[name];
      buf.set(blk.encode(), blk.base - base);
    }
    await this.p.wrMulti(base, buf);
  }

  // ── diagnostics ──
  /** Every field of the named blocks (default: all), one per line. */
  dump(names: readonly BlockName[] = BLOCK_NAMES): string {
    const lines: string[] = [];
    for (const name of names) {
      const blk = this.blocks[name];
      lines.push(
        `--- ${name} @ 0x${blk.base.toString(16).toUpperCase().padStart(4, "0")} (${blk.size} B)`,
      );
      for (const [field, value] of blk.items()) lines.push(`  ${field.padEnd(52)} ${value}`);
    }
    return lines.join("\n");
  }
}
