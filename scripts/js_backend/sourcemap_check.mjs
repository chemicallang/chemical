// Verifies a generated source map by decoding it.
//
//   node scripts/js_backend/sourcemap_check.mjs <bundle-dir> <source-file-name> <token>
//
// Parses the map the compiler wrote, decodes the "mappings" field (base64 VLQ,
// version 3), and checks that:
//   - the module links to the map and the map is well formed,
//   - every source index and source line is in range,
//   - at least one mapping lands on a line of <source-file-name> containing
//     <token>, i.e. the mapping actually points at the code that produced it.
// A map that merely exists but points nowhere would pass a smoke check and fail
// a debugger, so this decodes it.
import { readFileSync, readdirSync } from "node:fs";
import { basename, join } from "node:path";

const [dir, sourceName, token] = process.argv.slice(2);
if (!dir || !sourceName || !token) {
    console.error("usage: sourcemap_check.mjs <bundle-dir> <source-file-name> <token>");
    process.exit(2);
}

let failures = 0;
const check = (name, ok, detail) => {
    if (ok) {
        console.log(`ok   ${name}`);
    } else {
        failures++;
        console.error(`FAIL ${name}${detail ? ": " + detail : ""}`);
    }
};

const maps = readdirSync(dir).filter((f) => f.endsWith(".mjs.map"));
check("a source map was written", maps.length === 1, `found ${maps.length}`);
if (maps.length !== 1) process.exit(1);

const mapPath = join(dir, maps[0]);
const modulePath = join(dir, maps[0].replace(/\.map$/, ""));
const map = JSON.parse(readFileSync(mapPath, "utf8"));
const moduleText = readFileSync(modulePath, "utf8");

check("version 3", map.version === 3, String(map.version));
check("module links to the map", moduleText.trimEnd().endsWith(`sourceMappingURL=${basename(mapPath)}`));
check("sources are listed", Array.isArray(map.sources) && map.sources.length > 0);
check(
    "the source is the fixture",
    map.sources.some((s) => basename(s.replace(/\\/g, "/")) === sourceName),
    JSON.stringify(map.sources.slice(0, 4)),
);
check("mappings are not empty", typeof map.mappings === "string" && map.mappings.length > 0);

const sourceTexts = map.sources.map((s) => {
    try {
        return readFileSync(s, "utf8");
    } catch {
        return null;
    }
});
const sourceLinesPerFile = sourceTexts.map((t) => (t == null ? null : t.split("\n")));
check(
    "a listed source can be read back",
    sourceTexts.some((t) => t !== null),
    `${sourceTexts.filter((t) => t !== null).length}/${sourceTexts.length} readable`,
);

// ── base64 VLQ decode ──────────────────────────────────────────────────────
const B64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
function decodeSegment(text) {
    const fields = [];
    let value = 0;
    let shift = 0;
    for (const ch of text) {
        const digit = B64.indexOf(ch);
        if (digit < 0) throw new Error(`bad base64 char '${ch}'`);
        value += (digit & 0x1f) << shift;
        if (digit & 0x20) {
            shift += 5;
        } else {
            const negative = value & 1;
            let v = value >> 1;
            fields.push(negative ? -v : v);
            value = 0;
            shift = 0;
        }
    }
    return fields;
}

const sourceLines = null; // kept for clarity below; ranges are per file
let srcIndex = 0;
let srcLine = 0;
let srcCol = 0;
let mapped = 0;
let inRange = true;
let hitsToken = false;
let generatedLine = 0;
let malformed = false;

for (const lineText of map.mappings.split(";")) {
    if (lineText.length > 0) {
        for (const group of lineText.split(",")) {
            let fields;
            try {
                fields = decodeSegment(group);
            } catch (error) {
                malformed = true;
                break;
            }
            if (fields.length < 4) {
                malformed = true;
                continue;
            }
            srcIndex += fields[1];
            srcLine += fields[2];
            srcCol += fields[3];
            mapped++;
            if (srcIndex < 0 || srcIndex >= map.sources.length) {
                inRange = false;
                continue;
            }
            // Range is per source file: a line number is only meaningful against
            // the file it belongs to.
            const lines = sourceLinesPerFile[srcIndex];
            if (lines != null && (srcLine < 0 || srcLine >= lines.length)) inRange = false;
            if (lines != null && srcLine >= 0 && srcLine < lines.length) {
                if (lines[srcLine].includes(token)) hitsToken = true;
            }
        }
    }
    generatedLine++;
}

check("every segment decodes", !malformed);
check("there are mapped lines", mapped > 0, `${mapped}`);
check("source indices and lines are in range", inRange);
check("generated lines stay within the module", generatedLine <= moduleText.split("\n").length + 1);
check(`a mapping lands on the '${token}' line`, hitsToken, `decoded ${mapped} mappings`);

if (failures) {
    console.error(`\n${failures} source-map check(s) failed`);
    process.exit(1);
}
console.log(`\nsource map: ${mapped} mappings, all in range, '${token}' found`);
