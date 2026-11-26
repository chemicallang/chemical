// Node unit test for the wasm UI demo, using a tiny DOM shim.
// Run after building app.wasm:
//   node lang/compiled/wasm_ui/node_test.mjs
import assert from 'node:assert/strict';
import { fileURLToPath } from 'node:url';
import { loadFromFile } from '../../runtime/wasm/host.mjs';

class FakeNode {
    constructor(tag) {
        this.tagName = tag.toUpperCase();
        this.children = [];
        this.attrs = new Map();
        this.listeners = new Map();
        this.textContent = '';
    }
    appendChild(child) {
        this.children.push(child);
        child.parent = this;
        return child;
    }
    setAttribute(name, value) {
        this.attrs.set(name, value);
    }
    addEventListener(name, handler) {
        const list = this.listeners.get(name) ?? [];
        list.push(handler);
        this.listeners.set(name, list);
    }
    dispatch(name) {
        for (const handler of this.listeners.get(name) ?? []) handler();
    }
}

const document = {
    body: new FakeNode('body'),
    createElement: (tag) => new FakeNode(tag),
};

const wasmPath = fileURLToPath(new URL('./app.wasm', import.meta.url));
const app = await loadFromFile(wasmPath, { document, log: () => {} });

assert.equal(typeof app.instance.exports.start, 'function', 'start must be exported');
assert.equal(typeof app.instance.exports.chc_wasm_on_event, 'function', 'event dispatch must be exported');

app.instance.exports.start();

const root = document.body.children[0];
assert.ok(root, 'start appends a root element');
assert.equal(root.attrs.get('class'), 'wasm-counter');

const button = root.children[0];
const span = root.children[1];
assert.equal(button.tagName, 'BUTTON');
assert.equal(span.tagName, 'SPAN');
assert.equal(span.textContent, '0', 'initial counter is 0');

button.dispatch('click');
assert.equal(span.textContent, '1', 'click increments to 1');

button.dispatch('click');
button.dispatch('click');
assert.equal(span.textContent, '3', 'repeated clicks keep counting');

console.log('wasm_ui node test: OK');
