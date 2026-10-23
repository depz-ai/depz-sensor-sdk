/**
 * One type-aware rule for the SDK sources, run by CI (viewer-build job):
 *
 *   bunx eslint --no-config-lookup -c packages/depz-sensor-sdk-ts/eslint.async.config.mjs packages/depz-sensor-sdk-ts/src
 *
 * Every function that returns a Promise is `async`, so it reports a failure
 * by rejecting and never throws at the call (a caller that only attaches
 * `.catch()` would miss a synchronous throw).
 */

import tseslint from "typescript-eslint";

export default tseslint.config({
  files: ["packages/depz-sensor-sdk-ts/src/**/*.ts"],
  linterOptions: { reportUnusedDisableDirectives: "off" },
  languageOptions: {
    parser: tseslint.parser,
    parserOptions: {
      project: "packages/depz-sensor-sdk-ts/tsconfig.json",
      tsconfigRootDir: process.cwd(),
    },
  },
  plugins: { "@typescript-eslint": tseslint.plugin },
  rules: { "@typescript-eslint/promise-function-async": "error" },
});
