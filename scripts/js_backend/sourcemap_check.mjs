// Verifies generated source maps by decoding them.
//
//   node scripts/js_backend/sourcemap_check.mjs <bundle-dir> <source-file-name> <token>
//
// A bundle is one ES module per Chemical module, so it carries one map per
// module. Every map is checked structurally; the one that lists
// <source-file-name> must additionally map at least one generated line onto a
// line of that file containing <token>, i.e. the mapping points at the code that
// produced it. A map that merely exists but points nowhere would pass a smoke
// check and fail a debugger, so this decodes them.
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

const maps = readdirSync(dir).filter((f) => f.endsWith(".mjs.map")).sort();
check("at least one source map was written", maps.length > 0, `found ${maps.length}`);

let fixtureMapped = false;
let fixtureMap = null;
let totalMappings = 0;

for (const mapFile of maps) {
    const mapPath = join(dir, mapFile);
    const modulePath = mapPath.replace(/\.map$/, "");
    const map = JSON.parse(readFileSync(mapPath, "utf8"));
    let moduleText;
    try {
        moduleText = readFileSync(modulePath, "utf8");
    } catch {
        check(`${mapFile}: the module next to the map exists`, false);
        continue;
    }

    check(`${mapFile}: version 3`, map.version === 3, String(map.version));
    check(
        `${mapFile}: module links to the map`,
        moduleText.trimEnd().endsWith(`sourceMappingURL=${basename(mapPath)}`),
    );
    check(`${mapFile}: sources are listed`, Array.isArray(map.sources) && map.sources.length > 0);
    // An empty `mappings` is legitimate for a module whose functions carry no
    // source positions (compiler-synthesized ctor/dtor helpers); the bundle as a
    // whole must still map something, checked below.
    check(
        `${mapFile}: mappings field is a string`,
        typeof map.mappings === "string",
        typeof map.mappings,
    );

    const sourceTexts = map.sources.map((s) => {
        try {
            return readFileSync(s, "utf8");
        } catch {
            return null;
        }
    });
    const sourceLinesPerFile = sourceTexts.map((t) => (t == null ? null : t.split("\n")));
    check(
        `${mapFile}: a listed source can be read back`,
        sourceTexts.some((t) => t !== null),
        `${sourceTexts.filter((t) => t !== null).length}/${sourceTexts.length} readable`,
    );

    let srcIndex = 0;
    let srcLine = 0;
    let srcCol = 0;
    let mapped = 0;
    let inRange = true;
    let hitsToken = false;
    let malformed = false;
    let generatedLine = 0;

    for (const lineText of map.mappings.split(";")) {
        if (lineText.length > 0) {
            for (const group of lineText.split(",")) {
                let fields;
                try {
                    fields = decodeSegment(group);
                } catch {
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
                // A line number only means something against the file it belongs to.
                const lines = sourceLinesPerFile[srcIndex];
                if (lines != null && (srcLine < 0 || srcLine >= lines.length)) inRange = false;
                if (lines != null && srcLine >= 0 && srcLine < lines.length) {
                    if (lines[srcLine].includes(token)) hitsToken = true;
                }
            }
        }
        generatedLine++;
    }

    totalMappings += mapped;
    check(`${mapFile}: every segment decodes`, !malformed);
    check(`${mapFile}: source indices and lines are in range`, inRange);
    check(
        `${mapFile}: generated lines stay within the module`,
        generatedLine <= moduleText.split("\n").length + 1,
    );

    const namesSource = map.sources.some((s) => basename(s.replace(/\\/g, "/")) === sourceName);
    if (namesSource && (fixtureMap === null || hitsToken)) fixtureMap = mapFile;
    // A map lists the whole program's source files; only some of them have
    // mappings in *this* module. The token hit is therefore a bundle-wide
    // property, accumulated across maps.
    if (namesSource && hitsToken) fixtureMapped = true;
}

check("the bundle maps at least one line", totalMappings > 0, `${totalMappings}`);

check(
    `a map lists ${sourceName}`,
    fixtureMap !== null,
    fixtureMap === null ? maps.join(", ") : "",
);
check(
    `a mapping lands on the '${token}' line`,
    fixtureMapped,
    fixtureMap === null ? "no map for that source" : `in ${fixtureMap}, ${totalMappings} mappings total`,
);

if (failures) {
    console.error(`\n${failures} source-map check(s) failed`);
    process.exit(1);
}
console.log(
    `\nsource maps: ${maps.length} map(s), ${totalMappings} mappings, all in range, '${token}' found`,
);
