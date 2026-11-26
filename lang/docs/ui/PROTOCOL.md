# Chemical UI — debug protocol and CLI

**Status:** normative, v1 (`ui.debug.v1`) · **Date:** 2026-11-27
**Companions:** [`API.md`](API.md) §10, [`CONTRACTS.md`](CONTRACTS.md) §11,
[`HOST.md`](HOST.md) §5, [`EXECUTION.md`](EXECUTION.md) P5.1–P5.4.

The protocol is the primary agent interface. Screenshots are secondary. P5.1
must commit `protocol.schema.json` generated from this document; P5.4 must
conform to it.

---

## 1. Transport and security

- Native: newline-delimited JSON-RPC 2.0 over TCP on `127.0.0.1` (IPv4/IPv6
  loopback only). One request per line; one response per line.
- WASM: `globalThis.__chemicalUi.call(method, params) -> Promise`, plus optional
  WebSocket client to `chemical ui relay` (the CLI listens; the browser never
  listens).
- Discovery: `.chemical/ui/<pid>.json`, mode 0600, containing
  `{pid, port, token, started_ms, title, protocol:"1"}`. `--ui-pid-file`
  overrides.
- Auth: every request includes `"token"` (constant-time compare). `hello`
  carries it and returns capabilities; there is no unauthenticated method. WASM
  DOM bridge uses a page-scoped token injected at creation.
- Limits: max 4 MiB message, max depth 64, max 100 requests/s, 5 s per request,
  max 64 concurrent; screenshots chunked (`png.chunk {id, seq, data}`).
- No `eval`, no arbitrary filesystem writes outside the configured capture root
  (canonicalized, `O_NOFOLLOW`, 0700, atomic create), no network.
- Redaction: secure text values are `"••••"` everywhere unless the app started
  with `--ui-reveal-secrets`; logs/captures redact by default.
- The protocol thread parses and enqueues; all tree/surface work runs on the UI
  thread. A request that would block the UI beyond 5 s returns `Timeout`.

Error object:

```json
{"jsonrpc":"2.0","id":7,"error":{"code":-32602,"message":"invalid params","data":{"ui_code":"InvalidArgument","detail":"depth must be 0..64"}}}
```

`node_id` is an opaque string `"n<index>:<generation>"`; clients never parse it.
`key` is `"<label>#<id>:<tag>"` for display only; use `node_id`/`test_id` for
actions.

---

## 2. Methods (v1 minimum)

All methods from `API.md` §10, plus these mandatory additions:

| Method | Params | Returns |
|--------|--------|---------|
| `viewport.set` | `{w,h,scale?}` | `{}` (headless/real where supported) |
| `window.list` | `{}` | `[{window_id,title,size,scale,focused}]` |
| `window.open` / `window.close` | `{spec}` / `{window_id}` | `{window_id}` / `{}` |
| `theme.set` | `{mode:"light"\|"dark"\|"high_contrast"\|"system"}` | `{}` |
| `density.set` | `{density:"comfortable"\|"compact"}` | `{}` |
| `scale.set` | `{scale}` | `{}` |
| `hit_test` | `{x,y}` | `{node_id, chain:[node_id], clip, transform}` |
| `focus.get` | `{}` | `{node_id, chain}` |
| `focus.set` | `{node_id}` | `{}` |
| `scroll.get` / `scroll.set` | `{node_id}` / `{node_id, x, y}` | `{x,y,max_x,max_y}` / `{}` |
| `bounds.get` | `{node_id}` | `{x,y,w,h,clip}` |
| `tree.diff` | `{from_frame,to_frame?}` | added/removed/updated nodes |
| `wait.idle` | `{timeout_ms}` | `{frame, stable, pending}` |
| `error.list` | `{since?}` | `[{code,class,node_id,source,message,retry}]` |
| `fonts.coverage` | `{text,style?}` | `{missing:[codepoint], fallback:[family]}` |
| `fonts.request` | `{url,sha256}` | `{request_id}` (async) |
| `gpu.info` / `gpu.set` | `{}` / `{renderer,backend,prefer}` | `GpuInfo` / `{}` |
| `action.invoke` | `{node_id, action, value?}` | `{}` (`set_value` requires `value`) |
| `screenshot` | `{region?,scale?,background?,at_time?,include_overlay?}` | `{png_base64,width,height,frame,hash}` |
| `capture.start` | `{path, fps?, max_frames?, include_input?, include_screenshots?}` | `{capture_id}` |
| `state.snapshot` / `state.restore` | `{}` / `{version,state}` | `{version,state}` / `{}` |

`tree.snapshot` node fields: `node_id`, `key`, `test_id`, `role`, `name`,
`description`, `value` (redacted), `states`, `actions`, `bounds`, `source
{file,line,col}`, `component`, `slots`, `children`. `node.inspect` adds props,
resolved tokens, style, layout constraints, focus, hit path, component stack,
animations, image/font states.

`tree.find` supports `role`, `name` (substring/regex), `key`, `test_id`, `text`,
`component`, `state`, `within` (node_id), `all` (bool), `limit`.

`state.snapshot` required fields: `version`, `focus`, `scroll` (key→x,y), `texts`
(key→value, secure redacted), `theme`, `density`, `windows`, and optional
`app` (opaque by app hook). `state.restore` validates version, bounds, keys;
unknown keys are ignored; invalid values return `InvalidArgument`.

---

## 3. CLI

`chemical ui` is a self-built Chemical app (cached) launched by a tiny compiler
shim. Commands:

| Command | Purpose |
|---------|---------|
| `run [app] [--watch] [--debug] [--headless] [--renderer=cpu\|gpu]` | build/run/attach |
| `tree [--json] [--depth N] [--window ID]` | snapshot |
| `inspect <key\|test_id\|node_id>` | node details |
| `click <selector> [--index N]` / `type <selector> "text"` / `key <chord>` | act |
| `hover/drag/scroll <selector> …` | gestures |
| `shot --out FILE [--frame N] [--at MS] [--scale F] [--theme …]` | screenshot |
| `record --out DIR [--fps N] [--seconds S]` / `replay DIR` | capture |
| `diff A.png B.png [--max-diff-pixels N] [--yiq F]` | visual diff |
| `audit [--json]` / `bench [--json]` / `test [--filter]` | gates |
| `mcp` | MCP stdio adapter |
| `relay` | WASM WebSocket bridge |
| `catalog [--json] [--search Q]` / `tokens [--json]` | discoverability |
| `fonts add/list/remove` | cache management |

Selectors: `role=button name="Save"`, `test_id=save`, `key=action.save#12:0`,
`node=n12:3`, `within=...`. Exit codes: 0 success, 2 usage, 3 not found,
4 app/protocol error, 5 timeout, 6 gate failure. `--json` prints one JSON object
per command (not a human table) and never logs to stdout.

MCP tools are high-level, not one per RPC: `ui_snapshot`, `ui_find`,
`ui_inspect`, `ui_act`, `ui_clock`, `ui_screenshot`, `ui_capture`, `ui_audit`,
`ui_perf`, `ui_gpu`, `ui_fonts`, `ui_catalog`. Each returns markdown + JSON and
points to artifacts; errors are actionable.

---

## 4. Conformance and tests

- `protocol.schema.json` committed; every request/response validated.
- Fuzz: malformed NDJSON, huge lines, deep JSON, invalid node ids, negative
  scroll, NaN/Inf, path traversal, capture flood, token brute force.
- Security tests: non-loopback bind refused, wrong token, symlink capture,
  password redaction, WASM bridge origin/token.
- Determinism: screenshot hash at a fixed clock/font/scale; `capture`/`replay`
  reproduces frame hashes.
- A request during view/layout/paint must wait, not race; TSAN test.
