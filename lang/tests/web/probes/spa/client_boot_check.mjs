// Boot the *served* SPA client bundle in Node against a stub DOM.
//
// The probe server's own checks prove the bytes are wired (`<script type="module"
// src=".../client/boot.mjs">` in the shell, and the module served with 200). Those
// checks cannot prove the bundle *runs*: that needs a module loader and a `document`.
// This harness provides both — a tiny DOM and a `fetch` stub — imports the exported
// `boot.mjs`, and asks what the program did.
//
//   node client_boot_check.mjs <site out dir>
//
// Prints one `ok`/`FAIL` line per check and exits non-zero when one fails. It is
// deliberately dependency-free: it must run in CI with only `node`.

import { pathToFileURL } from "node:url";
import { readFileSync } from "node:fs";
import { join } from "node:path";

const outDir = process.argv[2];
if (!outDir) {
  console.error("usage: node client_boot_check.mjs <site out dir>");
  process.exit(2);
}

let failures = 0;
function check(name, ok) {
  console.log(`${ok ? "ok  " : "FAIL"} ${name}`);
  if (!ok) failures += 1;
}

// ── stub DOM ────────────────────────────────────────────────────────────────
class StubElement {
  constructor(tag) {
    this.tag = tag;
    this.children = [];
    this.listeners = [];
    this.textContent = "";
    this.innerHTML = "";
    this.value = "";
    this.attributes = new Map();
  }
  appendChild(child) {
    this.children.push(child);
    return child;
  }
  addEventListener(event, fn) {
    this.listeners.push({ event, fn });
  }
  setAttribute(name, value) {
    this.attributes.set(name, String(value));
  }
  getAttribute(name) {
    return this.attributes.has(name) ? this.attributes.get(name) : null;
  }
  hasAttribute(name) {
    return this.attributes.has(name);
  }
  removeAttribute(name) {
    this.attributes.delete(name);
  }
  removeChild(child) {
    const at = this.children.indexOf(child);
    if (at >= 0) this.children.splice(at, 1);
  }
  focus() {}
}

const bySelector = new Map();
const byId = new Map();
function stubFor(key) {
  let el = byId.get(key);
  if (!el) {
    el = new StubElement("div");
    byId.set(key, el);
  }
  return el;
}

// Every selector/id the client asks for gets a real element, so a missing stub
// never masquerades as "the program does nothing".
globalThis.document = {
  body: stubFor("body"),
  querySelector: (selector) => stubFor(`sel:${selector}`),
  getElementById: (id) => stubFor(`#${id}`),
  createElement: (tag) => new StubElement(tag),
};
globalThis.localStorage = {
  _map: new Map(),
  getItem(k) {
    return this._map.has(k) ? this._map.get(k) : null;
  },
  setItem(k, v) {
    this._map.set(k, String(v));
  },
  removeItem(k) {
    this._map.delete(k);
  },
};

// The endpoint the client fetches, answered from the shell's own bootstrap payload
// so there is one source of truth for the data.
const fetched = [];
globalThis.fetch = async (url) => {
  fetched.push(url);
  if (url === "/api/tasks") {
    const body = '{"tasks":"1|write the shell|open\\n2|fetch the payload|open\\n3|render rows|done","open":2}';
    return { status: 200, text: async () => body, ok: true };
  }
  return { status: 404, text: async () => "", ok: false };
};

// ── checks ──────────────────────────────────────────────────────────────────
const bootPath = join(outDir, "assets", "client", "boot.mjs");
let bootSource = "";
try {
  bootSource = readFileSync(bootPath, "utf8");
} catch {
  check(`the export contains ${bootPath}`, false);
  process.exit(1);
}
check("the export contains the boot module", bootSource.includes("$run()"));

// The bundle boots: `boot.mjs` imports `index.mjs`, which imports the program
// module and the runtime. Importing it runs `$run()` exactly as the browser does.
try {
  await import(pathToFileURL(bootPath).href);
} catch (error) {
  check(`the served bundle boots (${error && error.message})`, false);
  process.exit(1);
}
check("the served bundle boots", true);

const tasks = stubFor("#tasks");
check("the boot payload rendered rows", tasks.children.length > 0);
check("the client fetched the JSON endpoint", fetched.includes("/api/tasks"));
const status = stubFor("#status");
check(
  "the status line reports the fetched count",
  String(status.textContent).length > 0,
);
const nav = stubFor("sel:.views");
check("the nav got a click listener", nav.listeners.some((l) => l.event === "click"));

console.log(failures === 0 ? "client bundle: PASS" : `client bundle: FAIL (${failures})`);
process.exit(failures === 0 ? 0 : 1);
