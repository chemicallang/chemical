# Linux WebView Bridge: Current State & Redesign Proposal

## Executive Summary

The current Linux webview JS↔native bridge in `lang/libs/webview/posix/linux.ch` uses a **`prompt()`-based hack** that abuses the `script-dialog` WebKit signal to shuttle messages between JavaScript and native code. This works but is fragile, synchronous-only, and semantically incorrect.

The proper approach — already used by the [webview/webview](https://github.com/webview/webview) C++ reference library — uses WebKit2GTK's **`script-message-received` signal** with `window.webkit.messageHandlers.__webview__.postMessage()`. This is the designed-for API: zero dialogs, proper async support, and clean separation of concerns.

---

## 1. Current Implementation

### 1.1 Bridge JS (injected via `WebKitUserScript`)

```chemical
const WEBVIEW_BRIDGE_JS : *char = """window.webview_bridge = {
    call: function(method, args) {
        var r = prompt('__WV_BIND__' + method + String.fromCharCode(10) + args);
        if (r === null) return '{"ok":false,"error":"cancelled"}';
        return r;
    }
};"""
```

**How it works:**
1. JS calls `window.webview_bridge.call('echo', '{"msg":"hello"}')`
2. This calls `prompt('__WV_BIND__echo\n{"msg":"hello"}')` — a browser `prompt()` dialog
3. WebKit fires the `script-dialog` signal because JS called `prompt()`
4. The native signal handler (`linux_on_script_dialog`) intercepts it:
   - Checks if the message starts with `__WV_BIND__`
   - Splits on the newline separator to get `method` + `args`
   - Calls the bound native handler
   - Sets the prompt result text via `webkit_script_dialog_prompt_set_text(dialog, result.data())`
   - Returns `1` (handled) to suppress the actual dialog
5. `prompt()` returns the result string to JS

### 1.2 Signal Handler Registration (in `webview_bind`)

```chemical
g_signal_connect_data(
    wv.web_view as *mut void,
    "script-dialog",
    linux_on_script_dialog as *mut void,
    wv as *mut void,
    null,
    0)
```

### 1.3 Message Format

```
__WV_BIND__<method>\n<args_json>
```

The newline (`\n`) separator is safe because `JSON.stringify()` escapes raw newlines as `\n`.

---

## 2. Problems with the Current Approach

### 2.1 Semantic Misuse of `prompt()`

The `script-dialog` signal is designed for `alert()`, `confirm()`, and `prompt()` dialogs that WebKit shows to the user. Hijacking it for an RPC transport is a hack that:
- Breaks if the page actually calls `alert()` or `confirm()` (other code using the webview)
- Confuses WebKit's internal state tracking for dialog handling
- Makes it impossible to distinguish real user dialogs from bridge messages

### 2.2 Synchronous Only

`prompt()` is **synchronous** — it blocks the page's JavaScript until the native handler returns. This means:
- No async bridge calls (the reference library supports async via Promises)
- Long-running native handlers freeze the page
- No way to do fire-and-forget messaging

### 2.3 Fragile Protocol

- The `__WV_BIND__` prefix + newline separator is an ad-hoc protocol
- If a future WebKit version changes how `prompt()` return values are handled, it breaks
- No error handling beyond "prompt was cancelled" (`null`)

### 2.4 Invisible but Not Hidden

On some GTK themes/configurations, the `prompt()` dialog may briefly flash visible before the signal handler suppresses it. The current code returns `1` (handled) which tells WebKit not to show the dialog, but there's a race window.

---

## 3. The Proper Approach: `script-message-received`

### 3.1 How It Works

WebKit2GTK provides a first-class mechanism for JS→native messaging:

```
JavaScript                          Native (GTK main loop)
    │                                       │
    │  window.webkit.messageHandlers.       │
    │    __webview__.postMessage(msg)       │
    │──────────────────────────────────────►│
    │                                       │
    │                     script-message-received::__webview__
    │                                       │
    │                                       ├── Parse JSON
    │                                       ├── Call bound handler
    │                                       └── Build response
    │                                       │
    │◄──────────────────────────────────────│
    │  ExecuteScript("resolve(id, result)")
    │                                       │
```

### 3.2 Required APIs

These WebKit2GTK C functions are needed (all are in `libwebkit2gtk-4.0` / `libwebkitgtk-6.0`):

```c
// Register a named message handler on the content manager.
// Must be called BEFORE the page loads.
webkit_user_content_manager_register_script_message_handler(
    WebKitUserContentManager *manager,
    const gchar *name);

// Connect to the signal that fires when JS calls postMessage().
// The signal name is "script-message-received::<name>".
g_signal_connect_data(
    manager,
    "script-message-received::__webview__",
    G_CALLBACK(callback),
    user_data,
    NULL,
    0);

// The callback signature (for WebKit2GTK 4.x):
void callback(
    WebKitUserContentManager *manager,
    WebKitJavascriptResult *result,  // JS value from postMessage()
    gpointer user_data);
```

### 3.3 New Bridge JS

```javascript
window.webkit.messageHandlers.__webview__.postMessage(
    JSON.stringify({id: id, method: method, args: args})
);
```

The init script (injected via `AddScriptToExecuteOnDocumentCreated` equivalent) creates the `__webview__` object:

```javascript
(function() {
    'use strict';
    var _promises = {};
    function generateId() {
        var crypto = window.crypto || window.msCrypto;
        var bytes = new Uint8Array(16);
        crypto.getRandomValues(bytes);
        return Array.prototype.slice.call(bytes).map(function(n) {
            var s = n.toString(16);
            return ((s.length % 2) == 1 ? '0' : '') + s;
        }).join('');
    }
    window.__webview__ = {
        post: function(message) {
            window.webkit.messageHandlers.__webview__.postMessage(message);
        },
        call: function(method) {
            var _id = generateId();
            var _params = Array.prototype.slice.call(arguments, 1);
            var promise = new Promise(function(resolve, reject) {
                _promises[_id] = { resolve: resolve, reject: reject };
            });
            this.post(JSON.stringify({
                id: _id,
                method: method,
                params: _params
            }));
            return promise;
        },
        onReply: function(id, status, result) {
            var promise = _promises[id];
            if (!promise) return;
            delete _promises[id];
            if (result !== undefined) {
                try { result = JSON.parse(result); } catch(e) {
                    promise.reject(new Error("Failed to parse binding result as JSON"));
                    return;
                }
            }
            if (status === 0) { promise.resolve(result); }
            else { promise.reject(result); }
        },
        onBind: function(name) {
            if (window.hasOwnProperty(name)) {
                throw new Error('Property "' + name + '" already exists');
            }
            window[name] = (function() {
                var params = [name].concat(Array.prototype.slice.call(arguments));
                return window.__webview__.call.apply(this, params);
            });
        },
        onUnbind: function(name) {
            if (!window.hasOwnProperty(name)) {
                throw new Error('Property "' + name + '" does not exist');
            }
            delete window[name];
        }
    };
})();
```

### 3.4 Native Signal Handler (conceptual Chemical)

```chemical
// Callback for script-message-received::__webview__ signal.
// Extracts the message string from JSCValue and dispatches to on_message().
func linux_on_script_message(
    manager : *mut WebKitUserContentManager,
    js_result : *mut WebKitJavascriptResult,
    data : *mut void
) {
    var wv = data as *mut WebView
    if(wv == null || !wv.bind_handler_set || wv.bind_ctx == null) {
        return
    }
    // Extract string from JSCValue (same as existing js_result_to_string pattern)
    var jsc_value = webkit_javascript_result_get_js_value(js_result)
    if(jsc_value == null) { return }
    var msg_cstr = jsc_value_to_string(jsc_value)
    if(msg_cstr == null) { return }

    linux_on_message(wv, msg_cstr)
    free(msg_cstr as *mut void)
}

// Process an incoming bridge message (JSON from JS).
// Parses {id, method, params}, calls the bound handler, and sends
// the result back via ExecuteScript.
func linux_on_message(wv : *mut WebView, msg : *char) {
    var msg_view = std::string_view::make_no_len(msg)

    // Parse JSON: find "id":, "method":, "params":
    var id_val = parse_json_id(msg_view)
    var method = parse_json_method(msg_view)
    var params = parse_json_params(msg_view)

    // Call the bound handler
    var result = wv.bind_ctx.handler(method, params)

    // Send response back via ExecuteScript
    var js = string("window.__webview__.onReply(\"")
    js.append(id_val)
    js.append_view(string_view::make_no_len("\", 0, \""))
    js.append(escaped_result)
    js.append_view(string_view::make_no_len("\")"))
    webview_evaluate_js(wv, js.data())
}
```

### 3.5 Registration (in `webview_create` / initialization)

```chemical
// After creating the webview:
var manager = webkit_web_view_get_user_content_manager(wv.web_view as *mut WebKitWebView)

// Register the named message handler
webkit_user_content_manager_register_script_message_handler(manager, "__webview__")

// Connect the signal
g_signal_connect_data(
    manager as *mut void,
    "script-message-received::__webview__",
    linux_on_script_message as *mut void,
    wv as *mut void,
    null,
    0)

// Inject the init script (creates window.__webview__)
webview_inject_bridge(wv.web_view)
```

---

## 4. API Changes Required

### 4.1 New WebKit2GTK Extern Declarations

Add to `lang/libs/webview/posix/linux.ch`:

```chemical
// Register a named script message handler on the user content manager.
// Must be called before the page loads for the handler to receive messages.
@extern public func webkit_user_content_manager_register_script_message_handler(
    manager : *mut WebKitUserContentManager,
    name : *char
) : int

// WebKitJavascriptResult → JSCValue (already declared)
// jsc_value_to_string (already declared)
```

### 4.2 Remove

| What | Why |
|------|-----|
| `WEBVIEW_BIND_PREFIX` / `WEBVIEW_BIND_PREFIX_LEN` | No longer needed; the signal name handles routing |
| `WEBVIEW_BRIDGE_JS` (prompt-based) | Replaced with `postMessage`-based bridge |
| `webkit_script_dialog_get_dialog_type` | No longer intercepting script dialogs |
| `webkit_script_dialog_get_message` | No longer reading prompt messages |
| `webkit_script_dialog_prompt_set_text` | No longer setting prompt results |
| `WEBKIT_SCRIPT_DIALOG_PROMPT` constant | No longer checking dialog type |
| `linux_on_script_dialog` function | Replaced by `linux_on_script_message` |

### 4.3 New Extern Declarations

```chemical
// WebKitUserScript (already declared)
// WebKitUserContentManager (already declared)
// webkit_user_content_manager_add_script (already declared)
// webkit_web_view_get_user_content_manager (already declared)

// NEW: Register a named script message handler
@extern public func webkit_user_content_manager_register_script_message_handler(
    manager : *mut WebKitUserContentManager,
    name : *char
) : int
```

### 4.4 Modified Functions

| Function | Change |
|----------|--------|
| `webview_create` | Register `__webview__` handler + connect `script-message-received` signal |
| `webview_inject_bridge` | Replace `WEBVIEW_BRIDGE_JS` with init script that creates `window.__webview__` |
| `webview_bind` | Connect `script-message-received::__webview__` instead of `script-dialog` |
| `webview_destroy` | Disconnect `script-message-received` signal (automatic via widget destroy) |

---

## 5. Cross-Platform Alignment

### 5.1 Current State

| Platform | Bridge Transport | Response Direction | Async Support |
|----------|-----------------|-------------------|---------------|
| Linux | `prompt()` via `script-dialog` | Synchronous (prompt return) | No |
| Windows | `chrome.webview.postMessage()` via `WebMessageReceived` | `ExecuteScript` (deferred) | Yes (Promises) |

### 5.2 After Redesign

| Platform | Bridge Transport | Response Direction | Async Support |
|----------|-----------------|-------------------|---------------|
| Linux | `webkit.messageHandlers.__webview__.postMessage()` via `script-message-received` | `ExecuteScript` | Yes (Promises) |
| Windows | `chrome.webview.postMessage()` via `WebMessageReceived` | `ExecuteScript` (deferred) | Yes (Promises) |

### 5.3 Bridge JS Unification

After the redesign, both platforms use the same pattern:

```javascript
// Outbound (JS → Native):
// Linux:  window.webkit.messageHandlers.__webview__.postMessage(json)
// Windows: window.chrome.webview.postMessage(json)

// Inbound (Native → JS):
// Both:   ExecuteScript("window.__webview__.onReply(id, status, result)")
```

The init script creates a platform-agnostic `window.__webview__` object. Only the `post()` method differs per platform. This can be abstracted:

```javascript
var postFn = (function() {
    if (window.webkit && window.webkit.messageHandlers && window.webkit.messageHandlers.__webview__) {
        return function(msg) { window.webkit.messageHandlers.__webview__.postMessage(msg); };
    }
    if (window.chrome && window.chrome.webview) {
        return function(msg) { window.chrome.webview.postMessage(msg); };
    }
    return function(msg) { console.warn('No webview bridge available'); };
})();
```

---

## 6. Implementation Steps

### Phase 1: Add WebKit2GTK API declarations

1. Add `webkit_user_content_manager_register_script_message_handler` extern to `linux.ch`
2. Add `webkit_javascript_result_get_js_value` extern (if not already present for GTK4)
3. Keep existing `jsc_value_to_string` and `webkit_javascript_result_unref`

### Phase 2: Replace bridge JS

1. Replace `WEBVIEW_BRIDGE_JS` with the init script that creates `window.__webview__`
2. Update `webview_inject_bridge` to inject the new init script

### Phase 3: Replace signal handler

1. Replace `linux_on_script_dialog` with `linux_on_script_message`
2. Connect `script-message-received::__webview__` instead of `script-dialog`
3. Extract message from `WebKitJavascriptResult` → `JSCValue` → string (same pattern as existing `linux_js_callback`)
4. Parse JSON and dispatch to handler
5. Send response back via `webkit_web_view_run_javascript` (ExecuteScript equivalent)

### Phase 4: Update `webview_bind`

1. Connect `script-message-received::__webview__` signal on the user content manager
2. Register the `__webview__` message handler name
3. Remove `script-dialog` signal connection

### Phase 5: Update cleanup

1. Remove `script-dialog` related cleanup (signal auto-disconnects on widget destroy)
2. Ensure `script-message-received` signal is properly cleaned up

### Phase 6: Update Windows implementation to match

1. Align bridge JS structure with Linux (both use `window.__webview__` pattern)
2. Ensure both backends use the same JSON message format
3. Unify the init script per platform (only the `post()` method differs)

---

## 7. Migration Considerations

### 7.1 Breaking Changes

- None. The bridge is internal to the webview library. User code uses `webview_bind()` which works the same way.

### 7.2 Testing

- Existing `webview_lib_test` should continue to pass
- The `cdm` app (which uses `webview_bind`) should continue to work
- Create a new `webview_bind_test` that specifically tests the bridge with async patterns

### 7.3 Backward Compatibility

- If `script-dialog` is still connected (e.g., old code), both signals could fire. Ensure only one is connected per webview instance.
- The `prompt()` approach should be completely removed, not left as a fallback.

---

## 8. Reference

### WebKit2GTK Documentation

- `webkit_user_content_manager_register_script_message_handler()`: [WebKit2GTK 4.1 docs](https://webkitgtk.org/reference/webkit2gtk-4.1/method.UserContentManager.registerScriptMessageHandler.html)
- `script-message-received` signal: [WebKit2GTK 4.1 docs](https://webkitgtk.org/reference/webkit2gtk-4.1/class.UserContentManager.html#script-message-received)
- `webkit_web_view_run_javascript()`: [WebKit2GTK 4.1 docs](https://webkitgtk.org/reference/webkit2gtk-4.1/method.WebView.runJavascript.html)

### Reference Implementation

The [webview/webview](https://github.com/webview/webview) C++ library (MIT license) implements this exact pattern:
- `core/include/webview/detail/backends/gtk_webkitgtk.hh` — Linux backend
- `core/include/webview/detail/platform/linux/webkitgtk/compat.hh` — GTK version compat
- `core/include/webview/detail/engine_base.hh` — Cross-platform bridge JS and `on_message`

---

## 9. Risk Assessment

| Risk | Mitigation |
|------|-----------|
| `register_script_message_handler` requires the handler name before page load | Register in `webview_create`, before any `load_html`/`load_url` |
| GTK3 vs GTK4 API differences | Use compat wrapper (same approach as reference library) |
| Thread safety: signal handler runs on GTK main loop | Same as current implementation; handler must not block |
| `ExecuteScript` for response may not work during `script-message-received` callback | Same issue as Windows; may need deferred dispatch via `g_idle_add` |
