/**
 * Contract 08 «Line rules», shared by `.depzrec` and `.depzdata`: arrays and
 * objects nest at most 256 deep, the line's outermost value being level 1.
 * JSON.parse itself takes any depth, but the SDKs that parse by recursive
 * descent stop there, so every reader refuses the same lines.
 */
export const MAX_JSON_DEPTH = 256;

/**
 * Whether `line`, text of JSON.stringify, holds a lone surrogate: JSON.stringify
 * writes only those as `\\ud8xx`, and such a line is refused by the readers
 * whose strings are UTF-8 (Rust). An escaped backslash is matched as its own
 * escape, so the text `\\\\ud800` does not count.
 */
export function jsonHasLoneSurrogate(line: string): boolean {
  for (const m of line.matchAll(/\\(?:ud[89a-f][0-9a-f]{2}|[^])/g)) {
    if (m[0].length > 2) return true;
  }
  return false;
}

/** Whether `line` opens more than MAX_JSON_DEPTH arrays/objects at once (brackets in strings do not count). */
export function jsonTooDeep(line: string): boolean {
  let depth = 0;
  let inString = false;
  for (let i = 0; i < line.length; i++) {
    const c = line.charCodeAt(i);
    if (inString) {
      if (c === 0x5c) i++; // backslash: skip the escaped char
      else if (c === 0x22) inString = false;
    } else if (c === 0x22) {
      inString = true;
    } else if (c === 0x5b || c === 0x7b) {
      if (++depth > MAX_JSON_DEPTH) return true;
    } else if (c === 0x5d || c === 0x7d) {
      depth--;
    }
  }
  return false;
}
