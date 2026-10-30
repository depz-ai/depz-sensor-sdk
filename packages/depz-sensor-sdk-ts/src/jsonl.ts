/**
 * Contract 08 «Line rules», shared by `.depzrec` and `.depzdata`: arrays and
 * objects nest at most 256 deep, the line's outermost value being level 1.
 * JSON.parse itself takes any depth, but the SDKs that parse by recursive
 * descent stop there, so every reader refuses the same lines.
 */
export const MAX_JSON_DEPTH = 256;

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
