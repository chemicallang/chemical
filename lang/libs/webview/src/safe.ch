// safe — additive owning/borrowing façade over the webview API.
//
// The raw surface (`webview_create`, `webview_load_url(*char)`,
// `webview_evaluate_js_result(..., *mut void)`, `webview_bind`, …) lives under
// `webview::raw`. This file is the canonical normal-use surface:
//
//   * `webview::safe::WebView` is an owning handle created with
//     `WebView.new(&str, width, height)`;
//   * every text argument is a `&str`; results are owned `std::Option<string>`;
//   * `evaluate_result` takes a `std::function` callback (owned captures), no
//     user-data pointer;
//   * the handle is heap-backed, so it keeps a stable address across by-value
//     moves, and its private raw state pointer makes it `!Send`/`!Sync`.

public namespace webview {

public namespace safe {

using std::Option;
using std::Result;
using std::string;
using std::string_view;

public type EvalResultHandler = std::function<(result : Option<string>) => void>

// Heap box for one asynchronous evaluation callback. The native layer receives
// its address as user data; the trampoline deletes it after the callback runs.
@direct_init
struct EvalCallbackBox {
    var cb : EvalResultHandler
}

func eval_result_trampoline(data : *mut void, result : *char) { unsafe {
    var box = data as *mut EvalCallbackBox
    if(box == null) { return }
    var value : Option<string>
    if(result == null) {
        value = Option.None<string>()
    } else {
        var text = string()
        text.append_char_ptr(result)
        value = Option.Some<string>(text)
    }
    box.cb(value)
    delete box
}}

// ---------------------------------------------------------------------------
// Heap state (stable address for the lifetime of the native webview)
// ---------------------------------------------------------------------------

@direct_init
protected struct WebViewState {
    var inner : webview::WebView
    var destroyed : bool
}

// ---------------------------------------------------------------------------
// Owning handle
// ---------------------------------------------------------------------------

public struct WebView {

    private var state : *mut WebViewState

    public func new(title : &std::str, width : int = 800, height : int = 600) : Result<WebView, WebViewError> {
        unsafe {
            var st = new WebViewState
            if(st == null) {
                return Result.Err(WebViewError.InitFailed(string("webview: out of memory")))
            }
            memset(st as *mut void, 0, sizeof(WebViewState))
            st.inner = webview::WebView.make()
            st.inner.title = string()
            st.inner.title.append_view(title.to_view())
            st.inner.width = width
            st.inner.height = height
            st.destroyed = false

            var created = raw::webview_create(&raw mut st.inner)
            if(created is Result.Err) {
                var Err(err) = created else unsafe { unreachable }
                delete st
                return Result.Err(err)
            }
            return Result.Ok(WebView { state : st })
        }
    }

    // ── navigation / script ──────────────────────────────────────────────

    public func load_url(&mut self, url : &std::str) {
        unsafe {
            var st = self.state
            if(st == null || st.destroyed) { return }
            var text = string()
            text.append_view(url.to_view())
            raw::webview_load_url(&raw mut st.inner, text.data())
        }
    }

    public func load_html(&mut self, html : &std::str) {
        unsafe {
            var st = self.state
            if(st == null || st.destroyed) { return }
            var text = string()
            text.append_view(html.to_view())
            raw::webview_load_html(&raw mut st.inner, text.data())
        }
    }

    // Fire-and-forget script evaluation.
    public func evaluate(&mut self, script : &std::str) {
        unsafe {
            var st = self.state
            if(st == null || st.destroyed) { return }
            var text = string()
            text.append_view(script.to_view())
            raw::webview_evaluate_js(&raw mut st.inner, text.data())
        }
    }

    // Evaluate and deliver the engine's stringified result through `handler`.
    public func evaluate_result(&mut self, script : &std::str, handler : EvalResultHandler) {
        unsafe {
            var st = self.state
            if(st == null || st.destroyed) { return }
            var box = new EvalCallbackBox
            if(box == null) { return }
            memset(box as *mut void, 0, sizeof(EvalCallbackBox))
            box.cb = handler
            var text = string()
            text.append_view(script.to_view())
            raw::webview_evaluate_js_result(&raw mut st.inner, text.data(), eval_result_trampoline, box as *mut void)
        }
    }

    // ── title / geometry / visibility ────────────────────────────────────

    public func title(&self) : string {
        unsafe {
            var st = self.state
            if(st == null) { return string("") }
            return raw::webview_title(&raw mut st.inner)
        }
    }

    public func set_title(&mut self, title : &std::str) {
        unsafe {
            var st = self.state
            if(st == null || st.destroyed) { return }
            var text = string()
            text.append_view(title.to_view())
            raw::webview_set_title(&raw mut st.inner, text.data())
        }
    }

    public func set_size(&mut self, width : int, height : int) {
        unsafe {
            var st = self.state
            if(st == null || st.destroyed) { return }
            raw::webview_set_size(&raw mut st.inner, width, height)
        }
    }

    public func show(&mut self) {
        unsafe {
            var st = self.state
            if(st == null || st.destroyed) { return }
            raw::webview_show(&raw mut st.inner)
        }
    }

    public func hide(&mut self) {
        unsafe {
            var st = self.state
            if(st == null || st.destroyed) { return }
            raw::webview_hide(&raw mut st.inner)
        }
    }

    // Blocking UI loop. Returns when the window is closed or `stop()` runs.
    public func run(&mut self) {
        unsafe {
            var st = self.state
            if(st == null || st.destroyed) { return }
            raw::webview_run(&raw mut st.inner)
        }
    }

    public func stop(&mut self) {
        unsafe {
            var st = self.state
            if(st == null || st.destroyed) { return }
            raw::webview_stop(&raw mut st.inner)
        }
    }

    // ── JS <-> native bridge ─────────────────────────────────────────────

    // Register the single native bridge handler. The backend exposes ONE
    // handler (`window.__webview__.call(method, ...)`) that routes by method
    // name; there is no per-name registration in the native layer, so this
    // surface deliberately does not fake `bind(name, handler)`. Register one
    // handler and dispatch on `method` inside it. `unbind(name)` removes that
    // single handler and tells the page to stop routing `name`.
    public func bind(&mut self, handler : webview::JsBindHandler) : Result<std::Unit, WebViewError> {
        unsafe {
            var st = self.state
            if(st == null || st.destroyed) {
                return Result.Err(WebViewError.InitFailed(string("webview is destroyed")))
            }
            return raw::webview_bind(&raw mut st.inner, handler)
        }
    }

    public func unbind(&mut self, name : &std::str) {
        unsafe {
            var st = self.state
            if(st == null || st.destroyed) { return }
            var text = string()
            text.append_view(name.to_view())
            raw::webview_unbind(&raw mut st.inner, text.data())
        }
    }

    public func return_value(&mut self, id : &std::str, status : int, result : &std::str) {
        unsafe {
            var st = self.state
            if(st == null || st.destroyed) { return }
            var id_text = string()
            id_text.append_view(id.to_view())
            var result_text = string()
            result_text.append_view(result.to_view())
            raw::webview_return(&raw mut st.inner, id_text.data(), status, result_text.data())
        }
    }

    // ── UI-thread dispatch / native handle (explicitly unsafe) ───────────

    // Schedule `fn` to run on the UI thread. The callback and `arg` are raw
    // state: `arg` must stay valid until the callback runs, and `fn` must be a
    // plain function pointer (no capture).
    public unsafe func dispatch(&self, fn : webview::DispatchCallback, arg : *mut void) {
        unsafe {
            var st = self.state
            if(st == null || st.destroyed) { return }
            raw::webview_dispatch(&raw mut st.inner, fn, arg)
        }
    }

    // The platform handle for `kind` (NATIVE_HANDLE_*). Owned by the native
    // webview; it must not outlive it.
    public unsafe func native_handle(&self, kind : int) : *mut void {
        unsafe {
            var st = self.state
            if(st == null) { return null }
            return raw::webview_get_native_handle(&raw mut st.inner, kind)
        }
    }

    @delete
    func destruct(&self) {
        unsafe {
            var st = self.state
            if(st == null) { return }
            if(!st.destroyed) {
                raw::webview_destroy(&raw mut st.inner)
                st.destroyed = true
            }
            delete st
        }
    }
}

// A webview handle owns native UI thread state. Its private raw state pointer
// makes it structurally !Send/!Sync (the webview module does not import core's
// auto-trait names for an explicit negative impl).
} // namespace safe

} // namespace webview
