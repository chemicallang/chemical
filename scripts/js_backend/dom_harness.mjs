// Runs an emitted Chemical client bundle under Node against a small DOM.
//
//   node scripts/js_backend/dom_harness.mjs <bundle-dir>
//
// The JS backend targets the browser for `dom`, but a gate needs something it can
// run headlessly: this file installs `document`, `localStorage` and `fetch` on
// `globalThis`, imports the bundle, runs the program, then dispatches the clicks a
// real page would deliver. Everything the Chemical code touches is a plain JS object,
// so the check is about the *emitted JavaScript* (handle plumbing, callback wiring,
// string marshalling), not about this file's fidelity to the DOM.
import { pathToFileURL } from "node:url";
import { resolve } from "node:path";

class FakeElement {
  constructor(tagName, doc) {
    this.tagName = String(tagName).toUpperCase();
    this.ownerDocument = doc;
    this.attributes = new Map();
    this.children = [];
    this.parentNode = null;
    this.textContent = "";
    this.value = "";
    this.classList = new Set();
    this.listeners = new Map();
  }

  setAttribute(name, value) {
    const key = String(name);
    this.attributes.set(key, String(value));
    if (key === "class") this.classList = new Set(String(value).split(/\s+/).filter(Boolean));
    if (key === "id") this.ownerDocument.index.set(String(value), this);
  }

  getAttribute(name) {
    const key = String(name);
    return this.attributes.has(key) ? this.attributes.get(key) : null;
  }

  hasAttribute(name) { return this.attributes.has(String(name)); }
  removeAttribute(name) { this.attributes.delete(String(name)); }

  appendChild(child) {
    if (child.parentNode) child.parentNode.removeChild(child);
    child.parentNode = this;
    this.children.push(child);
    return child;
  }

  removeChild(child) {
    const i = this.children.indexOf(child);
    if (i >= 0) this.children.splice(i, 1);
    child.parentNode = null;
    return child;
  }

  addEventListener(type, handler) {
    const key = String(type);
    if (!this.listeners.has(key)) this.listeners.set(key, []);
    this.listeners.get(key).push(handler);
  }

  dispatchEvent(type) {
    const key = String(type);
    const event = {
      type: key,
      target: this,
      key: "",
      defaultPrevented: false,
      preventDefault() { this.defaultPrevented = true; },
      stopPropagation() {},
    };
    for (const handler of this.listeners.get(key) || []) handler(event);
    return event;
  }

  click() { return this.dispatchEvent("click"); }

  matches(selector) {
    const sel = String(selector).trim();
    if (sel.startsWith("#")) return this.getAttribute("id") === sel.slice(1);
    if (sel.startsWith(".")) return this.classList.has(sel.slice(1));
    return this.tagName === sel.toUpperCase();
  }

  querySelector(selector) {
    for (const child of this.children) {
      if (child.matches(selector)) return child;
      const found = child.querySelector(selector);
      if (found) return found;
    }
    return null;
  }

  focus() { this.ownerDocument.focused = this; }
}

class FakeDocument {
  constructor() {
    this.index = new Map();
    this.body = new FakeElement("body", this);
    this.documentElement = this.body;
    this.focused = null;
  }

  createElement(tag) { return new FakeElement(tag, this); }

  getElementById(id) {
    const key = String(id);
    if (this.index.has(key)) return this.index.get(key);
    const found = this.body.querySelector("#" + key);
    if (found) this.index.set(key, found);
    return found;
  }

  querySelector(selector) {
    if (this.body.matches(selector)) return this.body;
    return this.body.querySelector(selector);
  }
}

function build() {
  const doc = new FakeDocument();

  const app = doc.createElement("div");
  app.setAttribute("id", "app");

  const greeting = doc.createElement("span");
  greeting.setAttribute("id", "greeting");
  greeting.setAttribute("data-role", "hello");
  greeting.textContent = "hi";

  const name = doc.createElement("input");
  name.setAttribute("id", "name");

  const button = doc.createElement("button");
  button.setAttribute("id", "count-button");

  const count = doc.createElement("span");
  count.setAttribute("id", "count");
  count.textContent = "0";

  app.appendChild(greeting);
  app.appendChild(name);
  app.appendChild(button);
  app.appendChild(count);
  doc.body.appendChild(app);
  return doc;
}

const dir = process.argv[2];
if (!dir) {
  console.error("usage: node scripts/js_backend/dom_harness.mjs <bundle-dir>");
  process.exit(2);
}

globalThis.document = build();

const store = new Map();
globalThis.localStorage = {
  getItem: (key) => (store.has(String(key)) ? store.get(String(key)) : null),
  setItem: (key, value) => { store.set(String(key), String(value)); },
  removeItem: (key) => { store.delete(String(key)); },
};

globalThis.fetch = async (url) => ({
  status: 200,
  ok: true,
  text: async () => JSON.stringify({ ok: true, url: String(url) }),
});

const entry = pathToFileURL(resolve(dir, "index.mjs")).href;
let bundle;
try {
  bundle = await import(entry);
} catch (error) {
  console.error(`dom-harness: failed to load ${entry}`);
  console.error(error && error.stack ? error.stack : error);
  process.exit(1);
}

try {
  bundle.$run();
} catch (error) {
  console.error("dom-harness: the program threw");
  console.error(error && error.stack ? error.stack : error);
  process.exit(1);
}

// Deliver the clicks the page would deliver, then let the async fetch settle.
const button = globalThis.document.getElementById("count-button");
if (!button) {
  console.error("dom-harness: the page has no #count-button");
  process.exit(1);
}
button.click();
const countBeforeFetch = globalThis.document.getElementById("count");
if (!countBeforeFetch) {
  console.error("dom-harness: the page has no #count");
  process.exit(1);
}
button.click();

await new Promise((done) => setTimeout(done, 25));
console.log("dom-harness: done");
