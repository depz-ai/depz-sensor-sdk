/**
 * Live-hardware smoke of the TS SDK over Node serialport (task: verify the
 * library on the real ToF through the USB/IP-forwarded CDC port).
 *
 *   bun scripts/live-tof-smoke.ts [/dev/ttyACM0]
 *
 * Network link (USB/IP over Tailscale) has ~180 ms RTT: init takes ~30 s —
 * that's the channel, not a bug (see USBIP_CONNECT.md).
 */

import { NodeSerialTransport } from "../src/transport/node.js";
import { Vl53l8 } from "../src/sensors/vl53l8/vl53l8.js";
import { RESOLUTION_8X8 } from "../src/sensors/vl53l8/uld.js";

const port = process.argv[2] ?? "/dev/ttyACM0";
const t0 = Date.now();
const log = (msg: string) => console.log(`[${((Date.now() - t0) / 1000).toFixed(1)}s] ${msg}`);

const transport = new NodeSerialTransport(port);
// USB/IP link RTT ~180 ms — the 200 ms default timeout is borderline.
const dev = new Vl53l8(transport, { timeoutMs: 1000 });

try {
  await dev.open();
  const ident = await dev.identify();
  log(`identify: ${ident.softwareName} (${ident.sensorType}, fw ${ident.version})`);
  if (ident.sensorType !== "vl53l8") throw new Error("not a VL53L8 device");
  log(`device: ${await dev.getDeviceName()} serial: ${await dev.getSerialNumber()}`);
  log(`mcu temp: ${await dev.readMcuTemperature()} °C`);
  const sync = await dev.syncTime(5);
  log(`time sync: offset=${sync.offsetUs}us rtt=${sync.rttUs}us`);

  log("init('cx') — expect ~30 s over the network link...");
  await dev.init("cx", { progress: (s) => log(`  ${s}`) });
  await dev.setResolution(RESOLUTION_8X8);
  await dev.setRangingFrequencyHz(15);
  log(`resolution=${await dev.getResolution()} freq=${await dev.getRangingFrequencyHz()}`);

  const frames: number[][] = [];
  const done = new Promise<void>((resolve) => {
    const unsub = dev.onFrame((f) => {
      const d = Array.from(f.distanceMm.subarray(0, f.resolution));
      frames.push(d);
      log(
        `frame ${frames.length}: min=${Math.min(...d)} max=${Math.max(...d)} mm ` +
          `temp=${f.siliconTempDegc}C ts=${f.timestampUs}`,
      );
      if (frames.length >= 10) {
        unsub();
        resolve();
      }
    });
  });
  await dev.startRanging();
  log("streaming...");
  await done;
  await dev.stopRanging();
  const stats = dev.stats;
  log(
    `stats: rx=${stats.rxPackets} tx=${stats.txPackets} crcErr=${stats.crcErrors} ` +
      `hdrErr=${stats.headerErrors} seqGaps=${stats.seqGaps}`,
  );
  if (stats.crcErrors || stats.headerErrors || stats.seqGaps) {
    throw new Error("link errors detected");
  }
  log("LIVE SMOKE OK: 10 frames, clean link");
} finally {
  await dev.close();
}
