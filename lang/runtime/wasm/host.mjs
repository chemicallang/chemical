// Browser host for a Chemical wasm32 module.
//
// Instantiates the module with the `env` imports the freestanding runtime and
// the `browser` library expect. DOM nodes live in a JS-side handle table
// (0 = null, ids never reused), so Chemical only ever sees u32 handles and
// (ptr, len) scalar pairs.
//
// Usage in a page:
//   import { loadFromUrl } from './host.mjs';
//   const app = await loadFromUrl('./app.wasm');
//   app.instance.exports.start();
//
// Node tests can pass a DOM shim:
//   import { loadFromFile, createHost } from './host.mjs';

const EVENT_NAMES = { 1: 'click', 2: 'input', 3: 'change' };

export function createHost(options = {}) {
    const document = options.document ?? globalThis.document;
    const log = options.log ?? ((text) => console.log(text));

    const handles = new Map();
    let nextHandle = 1;
    let instance = null;

    function addHandle(value) {
        const id = nextHandle++;
        handles.set(id, value);
        return id;
    }

    function node(handle) {
        const value = handles.get(handle);
        if (value === undefined) {
            throw new Error(`wasm host: unknown DOM handle ${handle}`);
        }
        return value;
    }

    function readString(ptr, len) {
        const bytes = new Uint8Array(instance.exports.memory.buffer, ptr, len);
        return new TextDecoder().decode(bytes);
    }

    const imports = {
        env: {
            chc_log: (ptr, len) => log(readString(ptr, len)),
            chc_now_millis: () => Date.now(),

            chc_wasm_dom_body: () => addHandle(document.body),
            chc_wasm_dom_create_element: (tagPtr, tagLen) =>
                addHandle(document.createElement(readString(tagPtr, tagLen))),
            chc_wasm_dom_set_text: (handle, ptr, len) => {
                node(handle).textContent = readString(ptr, len);
            },
            chc_wasm_dom_append_child: (parent, child) => {
                node(parent).appendChild(node(child));
            },
            chc_wasm_dom_set_attribute: (handle, namePtr, nameLen, valuePtr, valueLen) => {
                node(handle).setAttribute(readString(namePtr, nameLen), readString(valuePtr, valueLen));
            },
            chc_wasm_dom_add_event_listener: (handle, eventId) => {
                const name = EVENT_NAMES[eventId];
                if (!name) throw new Error(`wasm host: unknown event id ${eventId}`);
                node(handle).addEventListener(name, () => {
                    if (typeof instance.exports.chc_wasm_on_event === 'function') {
                        instance.exports.chc_wasm_on_event(handle, eventId);
                    }
                });
            },
        },
    };

    async function instantiate(bytes) {
        const result = await WebAssembly.instantiate(bytes, imports);
        instance = result.instance;
        return result;
    }

    return {
        imports,
        instantiate,
        get instance() { return instance; },
        getNode: (handle) => handles.get(handle),
        handles,
    };
}

export async function loadFromFile(path, options = {}) {
    const fs = await import('node:fs');
    const host = createHost(options);
    await host.instantiate(fs.readFileSync(path));
    return host;
}

export async function loadFromUrl(url, options = {}) {
    const response = await fetch(url);
    if (!response.ok) throw new Error(`wasm host: fetch ${url} -> ${response.status}`);
    const host = createHost(options);
    await host.instantiate(await response.arrayBuffer());
    return host;
}
