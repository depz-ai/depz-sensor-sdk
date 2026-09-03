#!/usr/bin/env node
/**
 * typedoc-plugin-markdown emits a tree of .md files; this flattens that tree
 * into one committable Markdown file (parity with the Python single-file
 * api.md). Used for the root `docs/api.md` and, via `mergeMarkdownTree()`, for
 * each `docs/<sensor>/api.md` (see `scripts/gen-sensor-api-md.mjs`).
 *
 * Usage: node scripts/merge-api-md.mjs <generated-dir> <out-file>
 * The `docs:root` npm script points typedoc at a temp dir, then runs this and
 * removes the temp dir.
 */

import { readdirSync, readFileSync, writeFileSync, statSync, rmSync } from "node:fs";
import path from "node:path";

/** Recursively collect all .md files, relative to genDir. */
function walk(dir, base = dir) {
  const out = [];
  for (const name of readdirSync(dir)) {
    const full = path.join(dir, name);
    if (statSync(full).isDirectory()) out.push(...walk(full, base));
    else if (name.endsWith(".md")) out.push(path.relative(base, full));
  }
  return out;
}

/**
 * Flatten the typedoc-plugin-markdown tree at `genDir` into one Markdown
 * string, prefixed by `headerParts` (an array of lines). Returns
 * `{ text, count }`.
 *
 * A plain lexicographic sort groups each module's README ahead of its members
 * (README < classes/enumerations/functions/interfaces), which reads coherently.
 */
export function mergeMarkdownTree(genDir, headerParts) {
  const files = walk(genDir).sort();
  const parts = [...headerParts];
  for (const rel of files) {
    const body = readFileSync(path.join(genDir, rel), "utf8").trim();
    if (body.length === 0) continue;
    parts.push(body, "");
  }
  return { text: parts.join("\n") + "\n", count: files.length };
}

/** CLI: merge <generated-dir> into the root <out-file>, then delete the dir. */
function main() {
  const [, , genDir, outFile] = process.argv;
  if (!genDir || !outFile) {
    console.error("usage: merge-api-md.mjs <generated-dir> <out-file>");
    process.exit(2);
  }
  const header = [
    "# @depz/sensor-sdk — API reference",
    "",
    "Generated from the TypeScript sources by TypeDoc. Each sensor also has a " +
      "focused reference with just its own symbols: [SR04](sr04/api.md) · " +
      "[VL53L8CX](vl53l8cx/api.md) · [VL53L8CH](vl53l8ch/api.md) · " +
      "[BNO086](bno086/api.md). For the narrative guides see the per-sensor " +
      "`docs/<sensor>/` pages and `docs/overview.md`.",
    "",
  ];
  const { text, count } = mergeMarkdownTree(genDir, header);
  writeFileSync(outFile, text);
  rmSync(genDir, { recursive: true, force: true });
  console.log(`merged ${count} files → ${outFile}`);
}

if (import.meta.url === `file://${process.argv[1]}`) main();
