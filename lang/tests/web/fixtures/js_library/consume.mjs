// Node consumer for the JS library gate: import each emitted module, find the
// public functions through `$api` (the key is the mangled Chemical name), call
// them, and assert the output. `$exports` is the full symbol table, so a
// reachable private helper must be there but must not appear in `$api`.

import { readdirSync } from "node:fs";
import path from "node:path";
import { pathToFileURL } from "node:url";

const dir = process.argv[2];
if (!dir) {
    console.error("usage: consume.mjs <bundle-dir>");
    process.exit(2);
}

const files = readdirSync(dir).filter((f) => f.endsWith(".mjs") && f !== "runtime.mjs");
const api = new Map();
const exportKeys = new Set();

for (const file of files) {
    const mod = await import(pathToFileURL(path.join(dir, file)).href);
    for (const key of Object.keys(mod.$exports ?? {})) exportKeys.add(key);
    for (const [key, fn] of Object.entries(mod.$api ?? {})) {
        if (typeof fn === "function") api.set(key, fn);
    }
}

function apiFn(fragment) {
    for (const [key, fn] of api) {
        if (key.includes(fragment)) return { key, fn };
    }
    return null;
}

if (api.size === 0) {
    console.error("no $api table was emitted");
    process.exit(1);
}

const answer = apiFn("library_answer");
const sum = apiFn("library_sum");
const asyncAnswer = apiFn("library_async_answer");
if (!answer) {
    console.error("no $api entry for library_answer");
    process.exit(1);
}
if (!sum) {
    console.error("no $api entry for library_sum");
    process.exit(1);
}
if (!asyncAnswer) {
    console.error("no $api entry for library_async_answer");
    process.exit(1);
}

const hiddenInExports = [...exportKeys].some((key) => key.includes("library_hidden_helper"));
const hiddenInApi = [...api.keys()].some((key) => key.includes("library_hidden_helper"));
if (!hiddenInExports) {
    console.error("library_hidden_helper is missing from $exports (it is reachable)");
    process.exit(1);
}
if (hiddenInApi) {
    console.error("the non-public library_hidden_helper leaked into $api");
    process.exit(1);
}

const a = answer.fn();
if (a !== 42) {
    console.error(`library_answer() = ${a}, expected 42`);
    process.exit(1);
}
const s = sum.fn(9);
if (s !== 45) {
    console.error(`library_sum(9) = ${s}, expected 45`);
    process.exit(1);
}
const asyncValue = await asyncAnswer.fn();
if (asyncValue !== 43) {
    console.error(`library_async_answer() = ${asyncValue}, expected 43`);
    process.exit(1);
}

console.log(
    `library_answer=42 library_sum(9)=45 library_async_answer=43 ` +
    `hidden_in_exports=${hiddenInExports ? 1 : 0} hidden_in_api=${hiddenInApi ? 1 : 0} ` +
    `modules=${files.length}`
);
