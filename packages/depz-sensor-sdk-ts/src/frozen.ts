/**
 * Freeze a public constant table and the arrays and plain objects inside it,
 * so that no code — typed or not — can change how the SDK decodes. Internal.
 */
export function frozen<T>(table: T): T {
  if (typeof table === "object" && table !== null && !Object.isFrozen(table)) {
    Object.freeze(table);
    for (const inner of Object.values(table)) frozen(inner);
  }
  return table;
}
