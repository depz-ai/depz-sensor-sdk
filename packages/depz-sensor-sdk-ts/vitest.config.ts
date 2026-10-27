/**
 * Local vitest config. Its presence also stops vitest from walking up and
 * loading the repo-root vite.config.ts (the viewer app's TanStack config).
 */

import { defineConfig } from "vitest/config";

export default defineConfig({
  test: {
    include: ["test/**/*.test.ts"],
    coverage: {
      provider: "v8",
      include: ["src/**"],
      reporter: ["text-summary", "json-summary", "json"],
      reportsDirectory: "coverage",
      // A ratchet, not a goal: the floor is today's coverage rounded down, so
      // it can only go up. Raise these as tests land (the viewer runs on this
      // SDK; it is checked against the Python reference by contract 16, and
      // what it leaves untested is checked by nothing). Every module is at
      // >= 90 % lines; transport/webserial.ts is exercised with a stubbed
      // navigator.serial, transport/node.ts with a stubbed serialport.
      thresholds: {
        lines: 97,
        statements: 97,
        branches: 91,
        functions: 98,
      },
    },
  },
});
