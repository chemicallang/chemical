# WebView Library: API Gap Analysis & Implementation Plan

**Reference:** [webview/webview](https://github.com/webview/webview) C++ library (MIT license)  
**Date:** August 2026

---

## 1. Current API Surface

### Chemical Library (Windows + Linux)

| Function | Windows | Linux | Description |
|----------|:-------:|:-----:|-------------|
| `webview_create(wv)` | ✅ | ✅ | Initialize a WebView struct |
| `webview_destroy(wv)` | ✅ | ✅ | Destroy native resources |
| `webview_run(wv)` | ✅ | ✅ | Enter message loop |
| `webview_stop(wv)` | ✅ | ✅ | Quit message loop |
| `webview_show(wv)` | ✅ | ✅ | Show window |
| `webview_hide(wv)` | ✅ | ✅ | Hide window |
| `webview_set_title(wv, title)` | ✅ | ✅ | Set window title |
| `webview_set_size(wv, w, h)` | ✅ | ✅ | Set window size |
| `webview_load_url(wv, url)` | ✅ | ✅ | Navigate to URL |
| `webview_load_html(wv, html)` | ✅ | ✅ | Load HTML string |
| `webview_evaluate_js(wv, js)` | ✅ | ✅ | Evaluate JS (fire-and-forget) |
| `webview_evaluate_js_result(wv, js, cb, data)` | ✅ | ✅ | Evaluate JS with result callback |
| `webview_bind(wv, handler)` | ✅ | ✅ | Bind a single global JS↔native handler |
| `webview_set_bounds(wv, x, y, w, h)` | ✅ | ✅ | Set embed-mode bounds |
| `webview_attach(wv, parent, ...)` | ✅ | ✅ | Attach to existing window |
| `webview_window(wv)` | ✅ | ✅ | Get native Window handle |
| `webview_title(wv)` | ✅ | ✅ | Get title string |
| `webview_rebind(wv)` | ✅ | — | Re-store user data pointers |

### C++ Reference Library (Windows + Linux + macOS)

| Function | Description |
|----------|-------------|
| `webview_create(debug, window)` | Create with debug flag + optional parent window |
| `webview_destroy(w)` | Destroy |
| `webview_run(w)` | Enter event loop |
| `webview_terminate(w)` | Quit event loop (thread-safe) |
| `webview_dispatch(w, fn, arg)` | **Schedule function on UI thread (thread-safe)** |
| `webview_get_window(w)` | Get native window handle |
| `webview_get_native_handle(w, kind)` | **Get window/widget/browser controller handle** |
| `webview_set_title(w, title)` | Set title |
| `webview_set_size(w, w, h, hints)` | **Set size with hints (min/max/fixed/none)** |
| `webview_navigate(w, url)` | Navigate to URL |
| `webview_set_html(w, html)` | Load HTML |
| `webview_init(w, js)` | **Inject user script (runs on every page load)** |
| `webview_eval(w, js)` | Evaluate JS |
| `webview_bind(w, name, fn, arg)` | **Bind named JS function (per-name, not global)** |
| `webview_unbind(w, name)` | **Unbind a named function** |
| `webview_return(w, id, status, result)` | **Respond to async binding call (thread-safe)** |
| `webview_version()` | **Get library version info** |

---

## 2. Missing Features

### 2.1 `webview_dispatch` — Thread-Safe Function Scheduling

**Status: ✅ Implemented (Windows)** | **Linux: Stub (TODO)**

**Priority: HIGH** — Critical for multi-threaded applications.

**What it does:** Schedules a function to execute on the UI thread. The function is guaranteed to run inside the event loop, making it safe to access UI resources. Essential for background threads that need to update the webview.

**C++ reference:** `engine_base::dispatch(std::function<void()> f)` → calls `dispatch_impl(f)`:
- **Windows:** `PostMessageW(hwnd, WM_APP, 0, 0)` — stores the function in a queue, `WM_APP` handler drains it
- **Linux:** `g_idle_add` or `webkit_web_view_run_javascript` to schedule on the GTK main loop

**Implementation approach:**

```
// Cross-platform API
public func webview_dispatch(wv : *mut WebView, fn : (*mut void) => void, arg : *mut void)

// Windows: PostMessageW(WM_WV_APP) with fn+arg stored in a queue
// Linux: g_idle_add callback that calls fn(arg)
```

**Windows:**
- Already have `WM_WV_APP` used for deferred bridge responses
- Add a `dispatch_queue` (fixed-size array of `{fn, arg}` pairs) + `dispatch_count`
- `WM_WV_APP` handler drains both the JS queue and the dispatch queue
- `webview_dispatch` stores fn+arg and posts `WM_WV_APP`

**Linux:**
- Use `g_idle_add_full(G_PRIORITY_DEFAULT, callback, data, null)` on the GTK main loop
- The callback invokes `fn(arg)` then returns 0 (one-shot)
- Thread-safe because `g_idle_add` is designed for cross-thread scheduling

### 2.2 `webview_set_size` with Hints

**Status: ✅ Implemented (Windows)** | **Linux: Stub (TODO)**

**Priority: MEDIUM** — Needed for resize constraints.

**What it does:** In addition to setting default size, supports:
- `HINT_NONE` — default size (current behavior)
- `HINT_MIN` — minimum size constraint
- `HINT_MAX` — maximum size constraint
- `HINT_FIXED` — non-resizable window

**Current Chemical:** `webview_set_size(wv, width, height)` — always sets default size.

**C++ reference:** `webview_set_size(w, width, height, hints)`

**Implementation:**

```
public type SizeHint = enum { None, Min, Max, Fixed }

public func webview_set_size(wv : *mut WebView, width : int, height : int, hints : SizeHint)
```

**Windows:**
- Already has `wv_set_size(wv, width, height, hints)` internally with `WV_HINT_NONE/MIN/MAX/FIXED` constants
- `WV_HINT_MIN/MAX` → `SetWindowPos` with `SWP_NOMOVE` + min/max tracking
- `WV_HINT_FIXED` → removes `WS_THICKFRAME` style
- Just need to expose the `hints` parameter in the public API

**Linux:**
- `gtk_window_set_geometry_hints` (GTK3) for min/max/fixed
- GTK4 removed X11-specific hints — `HINT_MAX` is a no-op per C++ reference
- `HINT_FIXED` → `gtk_window_set_resizable(FALSE)` on GTK3/4

### 2.3 `webview_init` — User Script Injection

**Status: ✅ Implemented (Windows)** | **Linux: Stub (TODO)**

**Priority: MEDIUM** — Enables persistent JS extensions across page loads.

**What it does:** Injects JavaScript that runs on every page load (before `window.onload`). Unlike `evaluate_js`, which runs once on the current page, `init` scripts persist across navigations.

**C++ reference:** `engine_base::init(js)` → `add_user_script(js)`:
- **Windows:** `ICoreWebView2::AddScriptToExecuteOnDocumentCreated`
- **Linux:** `webkit_user_content_manager_add_script` with `WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START`

**Current Chemical:** No equivalent. The bridge init script is injected once during `webview_create` but there's no public API to add user scripts.

**Implementation:**

```
public func webview_init(wv : *mut WebView, js : *char)
```

**Windows:**
- Call `wv.webview.lpVtbl.AddScriptToExecuteOnDocumentCreated(wv.webview, js_wide, null)`
- Store the script key for later cleanup (optional)

**Linux:**
- Create `WebKitUserScript` with `WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START` and `WEBKIT_USER_SCRIPT_INJECT_INTO_ALL_FRAMES`
- Add via `webkit_user_content_manager_add_script(manager, script)`
- Store script reference for cleanup

### 2.4 Per-Name Bindings + `webview_unbind`

**Status: ✅ Implemented (Windows)** | **Linux: Stub (TODO)**

**Priority: LOW** — Current single-handler design works but limits composability.

**What it does:** The C++ library supports binding multiple named JS functions, each with its own native callback. `webview_unbind` removes a binding. Our current design uses a single global handler that dispatches by method name.

**C++ reference:**
- `webview_bind(w, name, fn, arg)` — creates `window.name = function(...args) { ... }`
- `webview_unbind(w, name)` — removes the binding, calls `onUnbind` in JS
- `webview_return(w, id, status, result)` — responds to an async binding call

**Current Chemical:** `webview_bind(wv, handler)` — single `JsBindHandler` that receives all calls.

**Assessment:** The current single-handler design is simpler and sufficient for most use cases. The handler can dispatch by method name internally. Per-name bindings would require:
- A `map<string, handler>` in the WebView struct
- Updated bridge JS to support `onBind`/`onUnbind` per name
- `webview_return` for async responses (thread-safe respond from background threads)

**If needed:** Implement as an optional API alongside the existing `webview_bind`:
```
public func webview_bind_name(wv : *mut WebView, name : *char, handler : JsBindHandler)
public func webview_unbind_name(wv : *mut WebView, name : *char)
public func webview_return(wv : *mut WebView, id : *char, status : int, result : *char)
```

### 2.5 `webview_get_native_handle`

**Status: ✅ Implemented (Windows)** | **Linux: Stub (TODO)**

**Priority: LOW** — Useful for advanced integrations.

**What it does:** Returns a specific native handle by kind:
- `HANDLE_WINDOW` — top-level window (GtkWindow/NSWindow/HWND)
- `HANDLE_WIDGET` — browser widget (GtkWidget/NSView/HWND)
- `HANDLE_BROWSER_CONTROLLER` — browser controller (WebKitWebView/WKWebView/ICoreWebView2Controller)

**Current Chemical:** `webview_window(wv)` returns the `Window` struct pointer (not the native HWND/GtkWindow).

**Implementation:**

```
public type NativeHandleKind = enum { Window, Widget, BrowserController }

public func webview_get_native_handle(wv : *mut WebView, kind : NativeHandleKind) : *mut void
```

**Windows:**
- `HANDLE_WINDOW` → `wv.win.hwnd`
- `HANDLE_WIDGET` → `wv.widget`
- `HANDLE_BROWSER_CONTROLLER` → `wv.controller`

**Linux:**
- `HANDLE_WINDOW` → `gtk_widget_get_toplevel(wv.widget)` (GTK3) or `gtk_window_get_default_widget` (GTK4)
- `HANDLE_WIDGET` → `wv.widget` (the GtkWidget hosting WebKit)
- `HANDLE_BROWSER_CONTROLLER` → `wv.web_view` (the WebKitWebView)

### 2.6 `webview_terminate` — Thread-Safe Stop

**Status: ✅ Already works** — `webview_stop` is thread-safe on both platforms.

**Priority: LOW** — `webview_stop` already works from any thread on Windows (PostQuitMessage). On Linux, need to verify.

**What it does:** Identical to `webview_stop` but explicitly documented as thread-safe.

**Current Chemical:** `webview_stop(wv)` — calls `window_quit()` on both platforms.

**Assessment:** Already works. Just needs documentation. On Windows, `PostQuitMessage` is thread-safe. On Linux, `gtk_main_quit` should be called from the main thread; if called from a background thread, `g_idle_add(gtk_main_quit, null)` is safer.

### 2.7 `webview_version` — Version Information

**Status: ✅ Implemented (Windows + Linux)**

**Priority: LOW** — Nice to have for diagnostics.

**Implementation:**
```
public struct WebViewVersion {
    var major : u32
    var minor : u32
    var patch : u32
    var version_string : string
}

public func webview_version() : WebViewVersion
```

### 2.8 Debug Mode Flag

**Priority: LOW** — Already partially implemented.

**What it does:** `webview_create(debug=1, ...)` enables DevTools on supported backends.

**Current Chemical:** `wv_init(wv, debug)` already takes a `debug` parameter. On Windows, it sets `AreDevToolsEnabled`. On Linux, it can set `webkit_settings_set_enable_developer_extras`.

**Missing:** The `webview::create()` wrapper doesn't expose the debug flag. Add an overload or parameter:
```
public func create(title : *char, width : int, height : int, debug : bool) : Result<WebView, WebViewError>
```

---

## 3. Cross-Platform Design Comparison

| Feature | C++ Reference | Chemical Current | Gap |
|---------|:------------:|:----------------:|:---:|
| Thread-safe dispatch | ✅ `webview_dispatch` | ❌ | HIGH |
| Size hints (min/max/fixed) | ✅ `hints` param | ❌ | MEDIUM |
| User script injection | ✅ `webview_init` | ❌ | MEDIUM |
| Per-name bindings | ✅ `webview_bind(name, ...)` | Single global handler | LOW |
| Binding unbind | ✅ `webview_unbind` | ❌ | LOW |
| Async return (thread-safe) | ✅ `webview_return` | Synchronous only | LOW |
| Native handle access | ✅ `get_native_handle` | `webview_window` only | LOW |
| Version info | ✅ `webview_version` | ❌ | LOW |
| Debug mode | ✅ `create(debug, ...)` | Internal only | LOW |
| Terminate (thread-safe) | ✅ `webview_terminate` | `webview_stop` (works) | NONE |

---

## 4. Implementation Recommendations

### Phase 1: High-Value Additions

1. **`webview_dispatch`** — Enables multi-threaded usage patterns. Most impactful missing feature.
2. **`webview_set_size` with hints** — The internal `wv_set_size` already supports hints; just expose the parameter.
3. **`webview_init`** — Enables user scripts and extensions.

### Phase 2: Completeness

4. **Debug flag in `webview::create`** — Minor API addition.
5. **`webview_version`** — Simple constant struct.
6. **`webview_get_native_handle`** — Enum-based accessor.

### Phase 3: Advanced (Optional)

7. **Per-name bindings + `webview_unbind`** — Only if the single-handler pattern proves limiting.
8. **`webview_return` for async binding responses** — Only if per-name bindings are added.

---

## 5. Platform-Specific Implementation Notes

### Windows (`win.ch`)

| Feature | Key APIs | Notes |
|---------|----------|-------|
| `dispatch` | `PostMessageW(WM_APP)`, queue drain in `wv_widget_proc` | Extend existing `WM_WV_APP` handler |
| Size hints | `SetWindowPos`, `GetWindowLongPtr(GWL_STYLE)` | Already implemented in `wv_set_size` |
| User scripts | `AddScriptToExecuteOnDocumentCreated` | Already used for bridge injection |
| Unbind | `RemoveScriptToExecuteOnDocumentCreated` | Store keys from `AddScript` |
| Version | Compile-time constants | `WEBVIEW_VERSION_MAJOR/MINOR/PATCH` |

### Linux (`posix/linux.ch`)

| Feature | Key APIs | Notes |
|---------|----------|-------|
| `dispatch` | `g_idle_add_full()` | Thread-safe GTK main loop scheduling |
| Size hints | `gtk_window_set_geometry_hints` (GTK3), `gtk_window_set_resizable` (GTK4) | GTK4 removed X11 hints |
| User scripts | `webkit_user_content_manager_add_script` | `WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START` |
| Unbind | `webkit_user_content_manager_remove_script` | Remove by matching script content |
| Bridge | `script-message-received` signal | Already using `prompt()` hack; migrate to proper signal |

---

## 6. macOS Status

The C++ reference library supports macOS via Cocoa/WKWebView. The Chemical webview library currently has **no macOS backend**. This is a significant gap if cross-platform coverage is a goal.

**Key macOS APIs needed:**
- `WKWebView` — browser widget
- `WKWebViewConfiguration` — configuration
- `WKUserContentController` — script injection + message handlers
- `NSWindow` / `NSViewController` — window management
- `WKScriptMessageHandler` protocol — bridge messages

**Effort:** High. The Cocoa/ObjC interop requires careful bridging. The C++ reference has ~500 lines of macOS-specific code. Consider this as a separate project.

---

## 7. Linux Bridge Migration (Completed)

The old `prompt()`-based bridge hack has been replaced with the canonical `window.__webview__` scheme matching the C++ reference. See the git history for details. The current Linux bridge uses:
- `script-dialog` signal for message interception (still a hack — should migrate to `script-message-received`)
- Same `window.__webview__.call()` / `onReply()` JS API as Windows
- Identical JSON message format `{id, method, params:[...]}`

**Remaining Linux bridge improvement:** Migrate from `script-dialog` to `script-message-received` signal. This eliminates the `prompt()` hack and enables true async messaging. See the deleted `webview-linux-bridge-redesign.md` for the full migration plan (now implemented on Windows; Linux uses the same bridge JS but still intercepts via `prompt()`).
