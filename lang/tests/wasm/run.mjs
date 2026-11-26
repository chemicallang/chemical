// Run a Chemical wasm module in Node and assert its observable result.
//
//   node run.mjs <app.wasm> [--expect-rc N] [--expect-text TEXT] [--main SYMBOL]
//
// Console bytes written through env.chc_log are collected and compared with
// --expect-text (substring). Exit status is 0 only when both assertions pass.
import fs from 'node:fs';

let file = null;
let expectRc = 0;
let expectText = null;
let mainName = 'main';

const args = process.argv.slice(2);
for (let i = 0; i < args.length; i++) {
    if (args[i] === '--expect-rc') expectRc = Number(args[++i]);
    else if (args[i] === '--expect-text') expectText = args[++i];
    else if (args[i] === '--main') mainName = args[++i];
    else file = args[i];
}
if (!file) {
    console.error('usage: node run.mjs <app.wasm> [--expect-rc N] [--expect-text TEXT] [--main SYMBOL]');
    process.exit(2);
}

let output = '';
const bytes = fs.readFileSync(file);
const { instance } = await WebAssembly.instantiate(bytes, {
    env: {
        chc_log: (ptr, len) => {
            output += Buffer.from(instance.exports.memory.buffer, ptr, len).toString('utf8');
        },
        chc_now_millis: () => Date.now(),
    },
});

const entry = instance.exports[mainName];
if (typeof entry !== 'function') {
    console.error(`wasm run: export '${mainName}' not found (exports: ${Object.keys(instance.exports).join(', ')})`);
    process.exit(1);
}

const rc = entry();
process.stdout.write(output);
if (expectText !== null && !output.includes(expectText)) {
    console.error(`wasm run: output does not contain ${JSON.stringify(expectText)}`);
    process.exit(1);
}
if (rc !== expectRc) {
    console.error(`wasm run: ${mainName}() returned ${rc}, expected ${expectRc}`);
    process.exit(1);
}
