// Runs an emitted Chemical JS program.
//
//   node scripts/js_backend/run.mjs <bundle-dir>
//
// The bundle's `index.mjs` exports `$run()`, which finds and calls the program's
// entry point. This driver exists so tests can run a bundle the same way a page
// would (import it, call the entry) without the bundle doing anything on import.
import { pathToFileURL } from "node:url";
import { resolve } from "node:path";

const dir = process.argv[2];
if (!dir) {
    console.error("usage: node scripts/js_backend/run.mjs <bundle-dir>");
    process.exit(2);
}

const entry = pathToFileURL(resolve(dir, "index.mjs")).href;
let bundle;
try {
    bundle = await import(entry);
} catch (error) {
    console.error(`js-backend: failed to load ${entry}`);
    console.error(error && error.stack ? error.stack : error);
    process.exit(1);
}

try {
    // `$run` is async: an async entry resolves only after its awaited I/O, so a
    // driver must wait for it rather than read a half-finished program.
    const rc = await bundle.$run();
    process.exitCode = typeof rc === "number" ? rc : 0;
} catch (error) {
    console.error("js-backend: the program threw");
    console.error(error && error.stack ? error.stack : error);
    process.exitCode = 1;
}
