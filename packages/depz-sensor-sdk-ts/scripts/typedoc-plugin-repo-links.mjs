/**
 * Local TypeDoc plugin: keep "Defined in" links only for files that are in the
 * repository.
 *
 * typedoc.json sets `disableGit`, so every source link is built from
 * `sourceLinkTemplate` with `{path}` relative to `displayBasePath` (the package
 * root) — the output no longer depends on whether `.git` is a directory (main
 * checkout), a file (worktree) or absent (tarball). Without git, TypeDoc can't
 * tell tracked files from the rest and would link inherited members declared
 * in `node_modules` (lib.es5.d.ts, @types/node, ...) to GitHub paths that
 * don't exist. This drops the URL of any source outside the package's own
 * files; its "Defined in" line stays, as plain text, as it was with git.
 */

import path from "node:path";
import { fileURLToPath } from "node:url";
import { Converter } from "typedoc";

const pkgRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");

/** True for a file the repository has under the package root. */
function inRepo(fullFileName) {
  const rel = path.relative(pkgRoot, fullFileName).replaceAll("\\", "/");
  return !rel.startsWith("../") && !path.isAbsolute(rel) && !rel.startsWith("node_modules/");
}

/** @param {import("typedoc").Application} app */
export function load(app) {
  // After SourcePlugin, which fills `source.url` when resolving begins.
  app.converter.on(Converter.EVENT_RESOLVE_END, (context) => {
    for (const refl of Object.values(context.project.reflections)) {
      for (const source of refl.sources ?? []) {
        if (source.url && !inRepo(source.fullFileName)) delete source.url;
      }
    }
  });
}
