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

  // `parentElement` skips a document node, and the document is what `body`
  // hangs from, so the distinction only matters there (`dom` maps it straight
  // through).
  get parentElement() {
    return this.parentNode && this.parentNode.tagName ? this.parentNode : null;
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
    // Pointer fields a real event carries: the typed `dom::Event` reads them, so a
    // harness event without them would silently answer 0 for every coordinate.
    const event = {
      type: key,
      target: this,
      key: "",
      clientX: 10,
      clientY: 20,
      button: 0,
      pointerId: 1,
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

  setPointerCapture(id) { globalThis.__chx_canvas_ops.push(`setPointerCapture(${id})`); }
  releasePointerCapture(id) { globalThis.__chx_canvas_ops.push(`releasePointerCapture(${id})`); }

  // A recording 2D context: every drawing call appends its arguments, so the gate
  // asserts the emitted JavaScript reached the host with the right numbers rather
  // than only that the Chemical side ran.
  getContext(kind) {
    if (String(kind) !== "2d") return null;
    if (!this.ctx) this.ctx = new FakeContext2D();
    return this.ctx;
  }
}

const canvasOps = [];
globalThis.__chx_canvas_ops = canvasOps;

class FakeContext2D {
  constructor() {
    this.fillStyle = "#000";
    this.strokeStyle = "#000";
    this.lineWidth = 1;
    this.font = "10px sans-serif";
  }
  fillRect(x, y, w, h) { canvasOps.push(`fillRect(${x},${y},${w},${h})`); }
  clearRect(x, y, w, h) { canvasOps.push(`clearRect(${x},${y},${w},${h})`); }
  beginPath() { canvasOps.push("beginPath"); }
  moveTo(x, y) { canvasOps.push(`moveTo(${x},${y})`); }
  lineTo(x, y) { canvasOps.push(`lineTo(${x},${y})`); }
  arc(x, y, r, s, e) { canvasOps.push(`arc(${x},${y},${r},${s},${e})`); }
  closePath() { canvasOps.push("closePath"); }
  fill() { canvasOps.push(`fill(${this.fillStyle})`); }
  stroke() { canvasOps.push(`stroke(${this.strokeStyle},${this.lineWidth})`); }
  save() { canvasOps.push("save"); }
  restore() { canvasOps.push("restore"); }
  fillText(text, x, y) { canvasOps.push(`fillText(${text},${x},${y})`); }
  translate(x, y) { canvasOps.push(`translate(${x},${y})`); }
  rotate(r) { canvasOps.push(`rotate(${r})`); }
  scale(x, y) { canvasOps.push(`scale(${x},${y})`); }
  setTransform(a, b, c, d, e, f) { canvasOps.push(`setTransform(${a},${b},${c},${d},${e},${f})`); }
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

// A real host promise: `/api/ok` succeeds, any other path answers 404, and
// `/api/down` *rejects* — the three outcomes an awaited `fetch` has to handle.
globalThis.fetch = async (url) => {
  const path = String(url);
  if (path.includes("/api/down")) throw new Error("network down");
  const status = path.includes("/api/ok") ? 200 : 404;
  return {
    status,
    ok: status === 200,
    text: async () => JSON.stringify({ ok: status === 200, url: path }),
  };
};

const rafQueue = [];
globalThis.requestAnimationFrame = (fn) => {
  rafQueue.push(fn);
  return rafQueue.length;
};

globalThis.cancelAnimationFrame = (id) => {
  if (id > 0 && id <= rafQueue.length) rafQueue[id - 1] = () => {};
};

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
  // `$run` is async: a client entry that awaits a host fetch resolves only after
  // the promise settles, and the harness must wait for it.
  await bundle.$run();
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

// `requestAnimationFrame` is delivered like a browser would: queued, then run with
// a frame timestamp. Printing the log proves the Chemical callback ran and that the
// drawing calls crossed the host boundary with their coordinates intact.
for (const frame of rafQueue.splice(0)) frame(16);
if (globalThis.__chx_canvas_ops.length) {
  console.log(`dom canvas ops=${globalThis.__chx_canvas_ops.join(";")}`);
}
console.log("dom-harness: done");
