/**
 * Local vitest config. Its presence also stops vitest from walking up and
 * loading the repo-root vite.config.ts (the viewer app's TanStack config).
 */

import { defineConfig } from "vitest/config";

export default defineConfig({
  test: {
    include: ["test/**/*.test.ts"],
  },
});
