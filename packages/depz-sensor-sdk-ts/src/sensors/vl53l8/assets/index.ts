/**
 * Lazy loader for the VL53L8 blob assets (sensor firmware + NVM/config
 * blobs). `loadAssets()` uses dynamic `import()` so the ~115 KB base64
 * modules stay out of bundles that never init the ToF sensor.
 */

export type Vl53l8Variant = "cx" | "ch";

export interface Vl53l8Assets {
  /** firmware.bin — 86016 bytes (0x15000). */
  firmware: Uint8Array;
  /** default_configuration.bin — 972 bytes. */
  defaultCfg: Uint8Array;
  /** default_xtalk.bin — 776 bytes. */
  defaultXtalk: Uint8Array;
  /** get_nvm_cmd.bin — 40 bytes. */
  getNvmCmd: Uint8Array;
}

export async function loadAssets(variant: Vl53l8Variant): Promise<Vl53l8Assets> {
  const mod = variant === "ch" ? await import("./ch.js") : await import("./cx.js");
  return mod.decode();
}

export { decodeBase64 } from "./decode.js";
