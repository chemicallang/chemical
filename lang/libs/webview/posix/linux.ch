// ---------------------------------------------------------------------------
// Linux webview using GTK3 + WebKit2GTK via C API
// ---------------------------------------------------------------------------
// All GTK/WebKit functions are declared as @extern (resolved at link time).
// The Chemical code manages all state, callbacks, and lifecycle.

public namespace webview {

using std::string;

// GTK types — reuse the window library's declarations so the two modules
// emit ONE set of extern C declarations (duplicate declarations of the same
// extern symbol with distinct Chemical types cause C redefinition errors).
// GtkBox/GtkFixed are webview-only containers, declared here.
public type GtkWindow = window::GtkWindow
public type GtkWidget = window::GtkWidget
public type GtkContainer = window::GtkContainer
@no_init @extern public struct GtkBox {}
@no_init @extern public struct GtkFixed {}
@no_init @extern public struct GtkBin {}
@no_init @extern public struct WebKitWebView {}
@no_init @extern public struct WebKitSettings {}
@no_init @extern public struct WebKitUserContentManager {}
@no_init @extern public struct WebKitUserScript {}

// GdkGeometry-based hints (gtk_window_set_geometry_hints) are not declared here
// because the system GTK header already provides that prototype with the real
// GdkGeometry type; redeclaring it causes a C redefinition error. The hint
// behavior below is implemented with already-declared GTK calls:
//   SIZE_HINT_MIN  -> gtk_widget_set_size_request (min size request)
//   SIZE_HINT_FIXED -> set_size_request + gtk_window_set_resizable(FALSE)
//   SIZE_HINT_MAX  -> default size (no simple GTK3 max-size API)
//   SIZE_HINT_NONE -> default size as usual
const GDK_HINT_MIN_SIZE : u32 = 2
const GDK_HINT_MAX_SIZE : u32 = 4
@no_init @extern public struct WebKitWebContext {}

// GTK constants
const GTK_WINDOW_TOPLEVEL = 0
const GTK_POS_LEFT = 0
const TRUE = 1
const FALSE = 0
const GTK_FILL = 4
const GTK_EXPAND = 2
const GTK_SHRINK = 1
const WEBKIT_LOAD_FINISHED = 4

// WebKit enum values (not exported by the runtime headers).
const WEBKIT_USER_CONTENT_INJECT_TOP_FRAME = 0
const WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START = 0

// JS<->native bridge transport. The JS side calls
//   window.webview_bridge.call(method, args)
// which posts JSON {id, method, args} to the native side through WebKit's
// script-message-received signal on the __webview__ user-content handler
// (window.webkit.messageHandlers.__webview__.postMessage). The native handler
// dispatches to the bound native function and returns the result by invoking
// window.__webview__.onReply(id, 0, result). This mirrors the WebView2
// (Windows) bridge contract so the same JS and the same tests run on every
// platform; only the postMessage transport differs. The bridge object is
// `window.__webview__` with `call(method, ...params)` returning a Promise that
// resolves with the JSON-parsed handler result; `window.webview_bridge` is kept
// as an alias for older callers.
const WEBVIEW_BRIDGE_JS : *char = """(function(){'use strict';function generateId(){var c=window.crypto||window.msCrypto;var b=new Uint8Array(16);c.getRandomValues(b);return Array.prototype.slice.call(b).map(function(n){var s=n.toString(16);return((s.length%2)==1?'0':'')+s;}).join('');}var Webview=(function(){var _p={};function W(){}W.prototype.post=function(m){return window.webkit.messageHandlers.__webview__.postMessage(m);};W.prototype.call=function(method){var _id=generateId();var _params=Array.prototype.slice.call(arguments,1);var promise=new Promise(function(resolve,reject){_p[_id]={resolve:resolve,reject:reject};});this.post(JSON.stringify({id:_id,method:method,params:_params}));return promise;};W.prototype.onReply=function(id,status,result){var promise=_p[id];if(!promise)return;if(result!==undefined){try{result=JSON.parse(result);}catch(e){promise.reject(new Error('Failed to parse binding result as JSON'));return;}}if(status===0){promise.resolve(result);}else{promise.reject(result);}};W.prototype._lrBuf=null;W.prototype._lrPush=function(chunk){if(!this._lrBuf)return;try{var obj=typeof chunk==='string'?JSON.parse(chunk):chunk;this._lrBuf.chunks.push(obj.data||'');}catch(e){this._lrBuf.chunks.push(String(chunk));}};W.prototype._lrDone=function(){if(!this._lrBuf)return;var lr=this._lrBuf;this._lrBuf=null;var full=lr.chunks.join('');var promise=_p[lr.id];if(promise){try{promise.resolve(JSON.parse(full));}catch(e){promise.reject(new Error('Failed to parse large result'));}}};W.prototype.onBind=function(name){if(window.hasOwnProperty(name)){throw new Error('Property "'+name+'" already exists');}window[name]=(function(){var params=[name].concat(Array.prototype.slice.call(arguments));return W.prototype.call.apply(this,params);}).bind(this);};W.prototype.onUnbind=function(name){if(!window.hasOwnProperty(name)){throw new Error('Property "'+name+'" does not exist');}delete window[name];};return W;})();window.__webview__=new Webview();window.webview_bridge=window.__webview__;})()"""

// The injected WebKitUserScript is owned by the user content manager for the
// lifetime of the web view (no manual unreference needed).

// GTK functions
@extern public func gtk_init(argc : *mut int, argv : *mut *mut *char) : int
@extern public func gtk_main()
@extern public func gtk_main_quit()
@extern public func gtk_main_iteration() : int
@extern public func gtk_events_pending() : int
@extern public func gtk_widget_show_all(widget : *mut GtkWidget)
@extern public func gtk_widget_destroy(widget : *mut GtkWidget)
@extern public func gtk_widget_set_size_request(widget : *mut GtkWidget, width : int, height : int)

@extern public func gtk_window_new(type_ : int) : *mut GtkWidget
@extern public func gtk_window_set_title(window : *mut GtkWindow, title : *char)
@extern public func gtk_window_set_default_size(window : *mut GtkWindow, width : int, height : int)
@extern public func gtk_window_set_position(window : *mut GtkWindow, position : int)
@extern public func gtk_window_set_resizable(window : *mut GtkWindow, resizable : int)
@extern public func g_idle_add(function : *mut void, data : *mut void) : u32
@extern public func gtk_window_get_type() : size_t

@extern public func gtk_box_new(orientation : int, spacing : int) : *mut GtkWidget
@extern public func gtk_box_pack_start(box : *mut GtkBox, child : *mut GtkWidget, expand : int, fill : int, padding : u32)

// GtkFixed: absolute-positioning container used by webview_attach to place the
// webview in a section of an existing app window/container
@extern public func gtk_fixed_new() : *mut GtkWidget
@extern public func gtk_fixed_put(fixed : *mut GtkFixed, widget : *mut GtkWidget, x : int, y : int)
@extern public func gtk_fixed_move(fixed : *mut GtkFixed, widget : *mut GtkWidget, x : int, y : int)
@extern public func gtk_widget_show(widget : *mut GtkWidget)
@extern public func gtk_widget_hide(widget : *mut GtkWidget)
@extern public func gtk_bin_get_child(bin : *mut GtkBin) : *mut GtkWidget

// WebKit functions
@extern public func webkit_web_view_new() : *mut GtkWidget
@extern public func webkit_web_view_get_type() : size_t
@extern public func webkit_web_view_load_uri(web_view : *mut WebKitWebView, uri : *char)
@extern public func webkit_web_view_load_html(web_view : *mut WebKitWebView, content : *char, base_uri : *char)
@extern public func webkit_web_view_run_javascript(web_view : *mut WebKitWebView, script : *char, cancellable : *mut void, callback : *mut void, user_data : *mut void)
@extern public func webkit_web_view_get_title(web_view : *mut WebKitWebView) : *char
@extern public func webkit_web_view_get_settings(web_view : *mut WebKitWebView) : *mut WebKitSettings

@extern public func webkit_settings_set_allow_file_access_from_file_urls(settings : *mut WebKitSettings, allowed : int)

@extern public func webkit_web_view_evaluate_javascript(web_view : *mut WebKitWebView, script : *char, length : isize, source_uri : *char, cancellable : *mut void, callback : *mut void, user_data : *mut void)

// GLib signal connection
// NOTE: g_signal_connect / g_signal_connect_swapped are header macros in C
// (they expand to g_signal_connect_data), so the actual exported symbol is
// g_signal_connect_data (lives in libgobject-2.0).
@extern public func g_signal_connect_data(instance : *mut void, signal : *char, handler : *mut void, data : *mut void, destroy_data : *mut void, connect_flags : int) : u64

// GMainLoop
@no_init @extern public struct GMainLoop {}
@extern public func g_main_loop_new(context : *mut void, is_running : int) : *mut GMainLoop
@extern public func g_main_loop_quit(loop : *mut GMainLoop)

// JavaScriptCore for result extraction
@no_init @extern public struct JSCValue {}
@no_init @extern public struct JSGlobalContextRef {}

@extern public func jsc_value_to_string(value : *mut JSCValue) : *char
@extern public func jsc_value_is_string(value : *mut JSCValue) : int

// WebKit async JS result (webkit_web_view_run_javascript_finish)
@no_init @extern public struct WebKitJavascriptResult {}
@extern public func webkit_web_view_run_javascript_finish(web_view : *mut WebKitWebView, res : *mut void, error : *mut *mut void) : *mut WebKitJavascriptResult
@extern public func webkit_javascript_result_get_js_value(result : *mut WebKitJavascriptResult) : *mut JSCValue
@extern public func webkit_javascript_result_unref(result : *mut WebKitJavascriptResult) : *mut WebKitJavascriptResult

// User content manager + script-message-received (JS<->native bridge) APIs
@extern public func webkit_web_view_get_user_content_manager(web_view : *mut WebKitWebView) : *mut WebKitUserContentManager
@extern public func webkit_user_content_manager_new() : *mut WebKitUserContentManager
@extern public func webkit_user_content_manager_add_script(manager : *mut WebKitUserContentManager, script : *mut WebKitUserScript)
@extern public func webkit_user_content_manager_register_script_message_handler(manager : *mut WebKitUserContentManager, name : *char) : int
@extern public func webkit_user_script_new(source : *char, injected_frames : int, injection_time : int, allow_list : *mut *mut void, block_list : *mut *mut void) : *mut WebKitUserScript

// Per-call context for webview_evaluate_js_result (freed in the callback).
public struct JsEvalHandler {
    var cb : JsResultCallback
    var user_data : *mut void
}

// ---------------------------------------------------------------------------
// WebView struct (Linux)
// ---------------------------------------------------------------------------

@direct_init
public struct WebView {
    // --- top-level window (standalone mode), owned by the window library
    //     (lang/libs/window). The webview fills the window; native UI can be
    //     mixed in through the window library. ---
    var win : window::Window

    var web_view : *mut GtkWidget
    var box : *mut GtkWidget

    // --- embed mode (webview_attach): the webview lives in a section of an
    //     existing app window::Window instead of its own top-level window.
    //     `fixed` is a GtkFixed (added to the app's window) that holds the
    //     webview at the section (bounds_x/y/w/h); native UI can live in the
    //     rest of the app's window. ---
    var attached : bool
    var parent_win : *mut window::Window
    var fixed : *mut GtkWidget
    var bounds_x : int
    var bounds_y : int
    var bounds_w : int
    var bounds_h : int

    var title : string
    var width : int
    var height : int
    var visible : bool
    var initialized : bool
    var bind_signal_connected : bool

    // Handler registered via webview_bind. Dispatches JS window.webview_bridge
    // calls. The handler is heap-allocated (std.function fields cannot be
    // initialized inside a struct literal); bind_ctx is null until bound and
    // bind_handler_set guards dispatch when no handler is bound.
    var bind_ctx : *mut JsBindHolder
    var bind_handler_set : bool

    @make
    func make() : WebView {
        return WebView {
            win : window::Window.make(),
            web_view : null,
            box : null,
            attached : false,
            parent_win : null,
            fixed : null,
            bounds_x : 0,
            bounds_y : 0,
            bounds_w : 0,
            bounds_h : 0,
            title : string("Chemical WebView"),
            width : 800,
            height : 600,
            visible : false,
            initialized : false,
            bind_signal_connected : false,
            bind_ctx : null,
            bind_handler_set : false
        }
    }
}

func linux_on_window_destroy(widget : *mut GtkWidget, data : *mut void) {
    gtk_main_quit()
}

func linux_on_navigation_complete(web_view : *mut WebKitWebView, event : int, data : *mut void) {
}

public func webview_create(wv : *mut WebView) : std::Result<std::Unit, WebViewError> {
    // WebKit's JavaScriptCore reads the process environment through glibc's
    // exported `environ`. The generated C references only `__environ`, and in
    // a non-PIE ELF that produces a copy relocation which leaves glibc's
    // `environ` NULL — JSC then segfaults walking its options. Mirror the real
    // environment pointer into `environ` before any WebKit object exists.
    comptime if(def.gnu) {
        environ = get_environ()
    }

    // The top-level window is created through the window library, so the app
    // can mix native UI with the webview (and gets all the window features).
    wv.win.width = wv.width
    wv.win.height = wv.height
    window::window_set_title(&raw mut wv.win, wv.title.data())
    var wres = window::window_create(&raw mut wv.win)
    if(wres is std::Result.Err) {
        return std.Result.Err(WebViewError.InitFailed(string("failed to create window")))
    }
    var window_widget = window::window_native_handle(&raw mut wv.win) as *mut GtkWidget

    // Create box layout
    wv.box = gtk_box_new(0, 0) // horizontal, no spacing

    // Create web view
    wv.web_view = webkit_web_view_new()
    if(wv.web_view == null) {
        return std.Result.Err(WebViewError.InitFailed(string("failed to create web view")))
    }

    // Configure web view settings
    // (JavaScript is always enabled in modern WebKitGTK (>= 2.40);
    // webkit_settings_set_javascript_enabled no longer exists there)
    var settings = webkit_web_view_get_settings(wv.web_view as *mut WebKitWebView)
    if(settings != null) {
        webkit_settings_set_allow_file_access_from_file_urls(settings, TRUE)
    }

    // Add web view to box
    gtk_box_pack_start(wv.box as *mut GtkBox, wv.web_view, TRUE, GTK_FILL, 0)

    // Add box to window
    gtk_container_add(window_widget as *mut GtkContainer, wv.box)

    wv.visible = false
    wv.initialized = true

    webview_inject_bridge(wv)

    return std.Result.Ok(std::Unit{})
}

// Inject the JS<->native bridge stub into every page the webview loads and wire
// up the script-message-received transport. The named handler (__webview__) must
// be registered on the user content manager BEFORE the page loads so that
// window.webkit.messageHandlers.__webview__ exists when the injected script runs
// (it executes at document start). The signal is connected once per webview.
func webview_inject_bridge(wv : *mut WebView) {
    if(wv == null) {
        return
    }
    var manager = webkit_web_view_get_user_content_manager(wv.web_view as *mut WebKitWebView)
    if(manager == null) {
        return
    }
    // Register the named message handler. Returns non-zero on success; calling
    // it again for the same name is harmless (WebKit returns 0 / FALSE).
    webkit_user_content_manager_register_script_message_handler(manager, "__webview__")
    // allow_list/block_list are NULL-terminated arrays of content-filter names;
    // null means "apply to all".
    var script = webkit_user_script_new(
        WEBVIEW_BRIDGE_JS,
        WEBKIT_USER_CONTENT_INJECT_TOP_FRAME,
        WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
        null, null)
    if(script != null) {
        webkit_user_content_manager_add_script(manager, script)
    }
}

// Heap-allocated holder for a bound JS<->native handler. Exists because a
// std.function field cannot be populated from inside a struct literal; the
// holder is `new`-allocated, assigned via set(), and `delete`d on destroy.
public struct JsBindHolder {
    var handler : JsBindHandler
    func set(&mut self, handler_ : JsBindHandler) {
        self.handler = handler_
    }
}

// Callback for the WebKitUserContentManager "script-message-received::__webview__"
// signal. The JS side posts a string via
// window.webkit.messageHandlers.__webview__.postMessage(); WebKit wraps it in a
// WebKitJavascriptResult. We extract the string, parse the {id, method, args}
// JSON, dispatch to the bound handler, and send the result back via
// window.webview_bridge._resolve(id, result).
func linux_on_script_message(
    manager : *mut WebKitUserContentManager,
    js_result : *mut WebKitJavascriptResult,
    data : *mut void
) : void {
    if(js_result == null || data == null) {
        return
    }
    var wv = data as *mut WebView
    if(!wv.bind_handler_set || wv.bind_ctx == null) {
        return
    }
    var jsc_value = webkit_javascript_result_get_js_value(js_result)
    if(jsc_value == null) {
        return
    }
    var msg_cstr = jsc_value_to_string(jsc_value)
    if(msg_cstr == null) {
        return
    }
    linux_on_message(wv, msg_cstr)
    free(msg_cstr as *mut void)
}

// Extract the JSON value for `key` (e.g. "\"params\":") from a bridge message.
// Mirrors the Windows webview_json_parse: quoted values are unescaped; raw
// values (object/array/number/bool/null) are copied until the top-level closing
// , } or ]. Returns "" if the key is not found.
func linux_json_value(msg_view : std::string_view, key : *char) : string {
    var pos = msg_view.find(std::string_view::make_no_len(key))
    if(pos >= msg_view.size()) {
        return string()
    }
    var vstart = pos + std::string_view::make_no_len(key).size()
    if(vstart >= msg_view.size()) {
        return string()
    }
    var c0 = msg_view.get(vstart)
    if(c0 == '"') {
        // Quoted JSON string: copy the content, unescaping escapes.
        vstart = vstart + 1
        var out = string()
        var i = vstart
        while(i < msg_view.size()) {
            var c = msg_view.get(i)
            if(c == '"') {
                break
            } else if(c == '\\') {
                i = i + 1
                if(i >= msg_view.size()) { break }
                var e = msg_view.get(i)
                if(e == 'n') { out.append('\n') }
                else if(e == 't') { out.append('\t') }
                else if(e == 'r') { out.append('\r') }
                else if(e == 'b') { out.append('\b') }
                else if(e == 'f') { out.append('\f') }
                else if(e == '/') { out.append('/') }
                else if(e == '\\') { out.append('\\') }
                else if(e == '"') { out.append('"') }
                else { out.append(e) }
            } else {
                out.append(c)
            }
            i = i + 1
        }
        return out
    }
    // Raw JSON value (object/array/number/bool/null): copy until the top-level
    // closing , } or ].
    var out = string()
    var depth = 0
    var i = vstart
    while(i < msg_view.size()) {
        var c = msg_view.get(i)
        if(depth == 0 && (c == ',' || c == '}' || c == ']')) {
            break
        }
        if(c == '{' || c == '[') {
            depth = depth + 1
            out.append(c)
        } else if(c == '}' || c == ']') {
            depth = depth - 1
            out.append(c)
        } else {
            out.append(c)
        }
        i = i + 1
    }
    return out
}

// Escape a string into a JSON string literal (with surrounding quotes). Used to
// embed the bridge call id and the handler's result into the reply script.
func linux_json_escape(sv : std::string_view) : string {
    var result = string("\"")
    var i : size_t = 0
    while(i < sv.size()) {
        var c = sv.get(i)
        if(c == '"') { result.append_view(std::string_view::make_no_len("\\\"")) }
        else if(c == '\\') { result.append_view(std::string_view::make_no_len("\\\\")) }
        else if(c == '\b') { result.append_view(std::string_view::make_no_len("\\b")) }
        else if(c == '\f') { result.append_view(std::string_view::make_no_len("\\f")) }
        else if(c == '\n') { result.append_view(std::string_view::make_no_len("\\n")) }
        else if(c == '\r') { result.append_view(std::string_view::make_no_len("\\r")) }
        else if(c == '\t') { result.append_view(std::string_view::make_no_len("\\t")) }
        else if((c as u8) <= (0x1f as u8)) {
            result.append_view(std::string_view::make_no_len("\\u00"))
            var hex = string("0123456789abcdef")
            var uc : u8 = c as u8
            result.append(hex.get((uc >> 4) as size_t))
            result.append(hex.get((uc & 0x0f) as size_t))
        } else {
            result.append(c)
        }
        i = i + 1
    }
    result.append('"')
    return result
}


// ---------------------------------------------------------------------------
// Large result chunking — bridge responses exceeding this threshold are
// stored in a C buffer and delivered to JS in small chunks.  The threshold
// is deliberately low so even escaped JSON stays under the WebKit script
// size limit (~64 KB).
// ---------------------------------------------------------------------------
const LARGE_RESULT_THRESHOLD : size_t = 32768

// Heap-allocated buffer holding the pending large result.  Protected by
// the GTK main-loop serialization (all access happens on the main thread
// inside signal handlers and their pump iterations).
unsafe var g_lr_buf : *char = null
unsafe var g_lr_size : size_t = 0
unsafe var g_lr_alloc : size_t = 0  // capacity of g_lr_buf
var g_lr_ready : bool = false

// State for the idle-delivery callback
struct LrIdleState {
    var wv : *mut WebView
    var call_id : string
}
unsafe var g_lr_idle_state : *mut LrIdleState = null

func linux_store_large_result(id_str : *char, data : *char, size : size_t) {
    // Free previous if any
    if(g_lr_buf != null) {
        free(g_lr_buf as *mut void)
    }
    g_lr_buf = malloc(size) as *char
    g_lr_alloc = size
    g_lr_size = size
    if(size > 0u && data != null) {
        memcpy(g_lr_buf as *mut void, data as *mut void, size)
    }
    g_lr_ready = true
}

func linux_chunk_result(offset : size_t, length : size_t) : string {
    if(!g_lr_ready || g_lr_buf == null || offset >= g_lr_size) {
        return string("{}")
    }
    var end = offset + length
    if(end > g_lr_size) { end = g_lr_size }
    var chunk_size = end - offset
    // Build a small JSON: {"data":"...escaped chunk...","offset":N,"total":M,"done":bool}
    var out = string("{\"data\":")
    var chunk_sv = std::string_view(g_lr_buf + offset, chunk_size)
    var escaped = linux_json_escape(chunk_sv)
    var escaped_sv = std::string_view::make_view(&escaped)
    out.append_view(&escaped_sv)
    out.append_view(std::string_view::make_no_len(",\"offset\":"))
    out.append_integer(offset as bigint)
    out.append_view(std::string_view::make_no_len(",\"total\":"))
    out.append_integer(g_lr_size as bigint)
    out.append_view(std::string_view::make_no_len(",\"done\":"))
    if(end >= g_lr_size) {
        out.append_view(std::string_view::make_no_len("true"))
    } else {
        out.append_view(std::string_view::make_no_len("false"))
    }
    out.append('}')
    return out
}

func linux_clear_large_result() {
    if(g_lr_buf != null) {
        free(g_lr_buf as *mut void)
        g_lr_buf = null
    }
    g_lr_size = 0
    g_lr_alloc = 0
    g_lr_ready = false
}

// Parse an integer field from a JSON params string.
func linux_json_int_field(params_view : std::string_view, key : *char) : size_t {
    var val_str = linux_json_value(params_view, key)
    if(val_str.size() == 0u) { return 0 }
    var result : size_t = 0
    var i : size_t = 0
    while(i < val_str.size()) {
        var c = val_str.get(i)
        if(c >= '0' && c <= '9') {
            result = result * 10 + (c - '0') as size_t
        }
        i = i + 1
    }
    return result
}


// Idle callback that delivers a large result in chunks via the main loop.
// This runs outside any signal handler, so webview_evaluate_js works normally.
func linux_lr_idle_deliver(data : *mut void) : i32 {
    fprintf(stderr, "[WV-BRIDGE] idle callback fired\n")
    var state = data as *mut LrIdleState
    if(state == null || state.wv == null || !g_lr_ready || g_lr_buf == null) {
        fprintf(stderr, "[WV-BRIDGE] idle callback: early exit (null state or no data)\n")
        if(state != null) { free(state as *mut void) }
        return 0
    }
    var wv = state.wv
        // 1. Init accumulator
    var init_js = string("window.__webview__._lrBuf={id:")
    var nid_sv = std::string_view::make_view(&state.call_id)
    var nid_view = nid_sv
    var esc_nid = linux_json_escape(nid_view)
    var esc_nid_view = std::string_view::make_view(&esc_nid)
    init_js.append_view(&esc_nid_view)
    init_js.append_view(std::string_view::make_no_len(",chunks:[],total:"))
    init_js.append_integer(g_lr_size as bigint)
    init_js.append_view(std::string_view::make_no_len(",offset:0}"))
    webview_evaluate_js(wv, init_js.data())

    // 2. Send chunks
    var chunk_offset : size_t = 0
    var chunk_size : size_t = 4096
    while(chunk_offset < g_lr_size) {
        if(chunk_size > g_lr_size - chunk_offset) { chunk_size = g_lr_size - chunk_offset }
        var chunk = linux_chunk_result(chunk_offset, chunk_size)
        var chunk_js = string("window.__webview__._lrPush(")
        var chunk_view = std::string_view::make_view(&chunk)
        var esc_chunk = linux_json_escape(chunk_view)
        var esc_chunk_view = std::string_view::make_view(&esc_chunk)
        chunk_js.append_view(&esc_chunk_view)
        chunk_js.append(')')
        webview_evaluate_js(wv, chunk_js.data())
        chunk_offset = chunk_offset + chunk_size
    }

    // 3. Assemble and resolve
    var done_js = string("window.__webview__._lrDone()")
    webview_evaluate_js(wv, done_js.data())

    // Cleanup
    linux_clear_large_result()
    free(state as *mut void)
    g_lr_idle_state = null
    return 0
}

// Process an incoming bridge message (JSON from JS). Parses {id, method,
// params}, calls the bound handler, and sends the result back via
// window.__webview__.onReply(id, 0, result).
func linux_on_message(wv : *mut WebView, msg : *char) {
    var msg_view = std::string_view::make_no_len(msg)

    var id_str = linux_json_value(msg_view, "\"id\":")
    var method = linux_json_value(msg_view, "\"method\":")
    var params_str = linux_json_value(msg_view, "\"params\":")
    var method_view = std::string_view::make_view(&method)

    // ---- Internal chunking protocol (intercept before user handler) ----
    // _getChunk: return a slice of a previously stored large result.
    var GET_CHUNK_SV = std::string_view::make_no_len("_getChunk")
    if(method_view.equals(&GET_CHUNK_SV)) {
        var params_view = std::string_view::make_view(&params_str)
        var offset_val = linux_json_int_field(params_view, "\"offset\":")
        var size_val = linux_json_int_field(params_view, "\"size\":")
        if(size_val == 0u) { size_val = 4096 }
        var chunk = linux_chunk_result(offset_val, size_val)
        var chunk_view = std::string_view::make_view(&chunk)
        var esc_chunk = linux_json_escape(chunk_view)
        var js_call = string("window.__webview__.onReply(")
        var id_view = std::string_view::make_view(&id_str)
        var esc_id = linux_json_escape(id_view)
        var esc_id_view = std::string_view::make_view(&esc_id)
        js_call.append_view(&esc_id_view)
        js_call.append_view(std::string_view::make_no_len(", 0, "))
        var esc_chunk_view = std::string_view::make_view(&esc_chunk)
        js_call.append_view(&esc_chunk_view)
        js_call.append(')')
        webview_evaluate_js(wv, js_call.data())
        return
    }

    if(method.size() == 0) {
        return
    }

    var handler_params = std::string_view::make_view(&params_str)
    var result = wv.bind_ctx.handler(method_view, handler_params)

    // For large results (>32KB), store in a C buffer and schedule delivery
    // via an idle callback. This avoids re-entrancy issues with
    // webview_evaluate_js being called from inside a signal handler.
    if(result.size() >= LARGE_RESULT_THRESHOLD) {
        linux_store_large_result(id_str.data(), result.data(), result.size())
        var state = malloc(sizeof(LrIdleState)) as *mut LrIdleState
        state.wv = wv
        state.call_id = string("")
        var id_sv = std::string_view::make_view(&id_str)
        state.call_id.append_view(&id_sv)
        g_lr_idle_state = state
        g_idle_add(linux_lr_idle_deliver as *mut void, state as *mut void)
        return
    }
    // Small result: send inline.
    var js_call = string("window.__webview__.onReply(")
    var id_view = std::string_view::make_view(&id_str)
    var esc_id = linux_json_escape(id_view)
    var esc_id_view = std::string_view::make_view(&esc_id)
    js_call.append_view(&esc_id_view)
    js_call.append_view(std::string_view::make_no_len(", 0, "))
    if(result.size() == 0) {
        js_call.append_view(std::string_view::make_no_len("undefined"))
    } else {
        var result_view = std::string_view::make_view(&result)
        var esc_result = linux_json_escape(result_view)
        var esc_result_view = std::string_view::make_view(&esc_result)
        js_call.append_view(&esc_result_view)
    }
    js_call.append(')')
    webview_evaluate_js(wv, js_call.data())
}

// Bind a native handler to the JS window.webview_bridge.call(method, args).
// The handler runs on the GTK main loop; it must not block for long. The
// script-message-received signal is connected here, after the caller owns the
// final WebView address returned by create().
public func webview_bind(wv : *mut WebView, handler : JsBindHandler) : std::Result<std::Unit, WebViewError> {
    if(wv.web_view == null) {
        return std.Result.Err(WebViewError.InitFailed(string("webview_bind: webview is not initialized")))
    }
    var holder = new JsBindHolder
    if(holder == null) {
        return std.Result.Err(WebViewError.InitFailed(string("webview_bind: allocation failed")))
    }
    // `new` does not construct members, so the std.function capture in the
    // freshly allocated holder is garbage. Zeroing makes the implicit delete of
    // the old capture a no-op (dtor null, is_heap false) before set() copies
    // the real handler in.
    memset(holder as *mut void, 0, sizeof(JsBindHolder))
    holder.set(handler)
    wv.bind_ctx = holder
    wv.bind_handler_set = true
    if(!wv.bind_signal_connected) {
        var manager = webkit_web_view_get_user_content_manager(wv.web_view as *mut WebKitWebView)
        if(manager == null) {
            delete holder
            wv.bind_ctx = null
            wv.bind_handler_set = false
            return std.Result.Err(WebViewError.InitFailed(string("webview_bind: user content manager unavailable")))
        }
        g_signal_connect_data(
            manager as *mut void,
            "script-message-received::__webview__",
            linux_on_script_message as *mut void,
            wv as *mut void,
            null,
            0)
        wv.bind_signal_connected = true
    }
    return std.Result.Ok(std::Unit{})
}

// NOTE: @extern does not take a separate linkage name — the emitted C symbol
// is the function's own name (no-mangled), which is exactly `gtk_container_add`
// here, so a plain @extern is correct.
@extern
func gtk_container_add(container : *mut GtkContainer, child : *mut GtkWidget)

// ===========================================================================
// public API: embed the webview into a section of an existing app window
// ===========================================================================

// Attach the webview to an app-owned window::Window instead of creating its
// own top-level window. The webview occupies the section (x, y, width, height)
// inside a GtkFixed that is added to the app's window; native UI can live in
// the rest of the app's window. Move the section with webview_set_bounds. The
// parent must already be created via window::window_create.
public func webview_attach(
    wv : *mut WebView,
    parent : *mut window::Window,
    x : int,
    y : int,
    width : int,
    height : int
) : std::Result<std::Unit, WebViewError> {
    if(parent == null) {
        return std.Result.Err(WebViewError.InitFailed(string("webview_attach: parent window is null")))
    }
    if(!window::window_is_created(parent)) {
        return std.Result.Err(WebViewError.InitFailed(string("webview_attach: parent window is not created (call window::window_create first)")))
    }
    if(wv.attached) {
        return std.Result.Err(WebViewError.InitFailed(string("webview_attach: webview is already attached")))
    }
    // NOTE: no gtk_init() here — the app must already have initialized GTK to
    // own the parent window, and gtk_init may only be called once per process
    // (a second call logs a GTK warning).

    wv.attached = true
    wv.parent_win = parent
    wv.bounds_x = x
    wv.bounds_y = y
    wv.bounds_w = width
    wv.bounds_h = height

    wv.web_view = webkit_web_view_new()
    if(wv.web_view == null) {
        wv.attached = false
        wv.parent_win = null
        return std.Result.Err(WebViewError.InitFailed(string("failed to create web view")))
    }
    var settings = webkit_web_view_get_settings(wv.web_view as *mut WebKitWebView)
    if(settings != null) {
        webkit_settings_set_allow_file_access_from_file_urls(settings, TRUE)
    }

    // Absolute-position the webview inside a GtkFixed so it occupies its
    // section of the app's existing window.
    var parent_widget = window::window_native_handle(parent) as *mut GtkWidget
    wv.fixed = gtk_fixed_new()
    gtk_widget_set_size_request(wv.web_view, width, height)
    gtk_fixed_put(wv.fixed as *mut GtkFixed, wv.web_view, x, y)
    // A GtkWindow is a GtkBin: it allows ONE direct child. If the app already
    // packed native widgets into a content container (box/grid/fixed), add the
    // webview section INTO that container so native UI and the webview coexist
    // in the same window; otherwise add it directly to the window. NOTE: to
    // mix native UI with the webview, the window's sole child must be a
    // container (e.g. a GtkBox) — a non-container child would trigger a GTK
    // warning instead.
    var host : *mut GtkWidget = parent_widget
    var existing_child = gtk_bin_get_child(parent_widget as *mut GtkBin)
    if(existing_child != null) {
        host = existing_child
    }
    gtk_container_add(host as *mut GtkContainer, wv.fixed)
    gtk_widget_show_all(wv.fixed)

    webview_inject_bridge(wv)

    wv.initialized = true
    return std.Result.Ok(std::Unit{})
}

// Move/resize the webview section inside the attached parent window (in the
// parent's coordinate space). No-op in standalone mode.
public func webview_set_bounds(wv : *mut WebView, x : int, y : int, width : int, height : int) {
    if(!wv.attached) {
        return
    }
    wv.bounds_x = x
    wv.bounds_y = y
    wv.bounds_w = width
    wv.bounds_h = height
    if(wv.fixed != null && wv.web_view != null) {
        gtk_fixed_move(wv.fixed as *mut GtkFixed, wv.web_view, x, y)
        gtk_widget_set_size_request(wv.web_view, width, height)
    }
}

public func webview_destroy(wv : *mut WebView) {
    wv.initialized = false
    if(wv.bind_ctx != null) {
        delete wv.bind_ctx
    }
    wv.bind_ctx = null
    wv.bind_handler_set = false
    wv.bind_signal_connected = false
    // If the last window_run() returned because the window was destroyed (the
    // user closed it), every widget packed into it — including this webview
    // section and the top-level window itself — has already been finalized.
    // Destroying them again would hit gtk_widget_destroy assertions. In the
    // standalone by-value case the embedded wv.win may also still hold stale
    // pointers (the struct can be relocated by webview::create), so just
    // clear the state.
    if(window::window_quit_by_destroy() != 0) {
        if(wv.bind_ctx != null) {
            delete wv.bind_ctx
        }
        wv.bind_ctx = null
        wv.bind_handler_set = false
        wv.web_view = null
        wv.fixed = null
        wv.attached = false
        wv.parent_win = null
        wv.win.created = false
        wv.win.widget = null
        wv.win.visible = false
        return
    }
    if(wv.attached) {
        // embed mode: destroy only the webview + fixed container; the app owns
        // the parent window.
        if(wv.web_view != null) {
            gtk_widget_destroy(wv.web_view)
            wv.web_view = null
        }
        if(wv.fixed != null) {
            gtk_widget_destroy(wv.fixed)
            wv.fixed = null
        }
        wv.attached = false
        wv.parent_win = null
        return
    }
    if(wv.web_view != null) {
        gtk_widget_destroy(wv.web_view)
        wv.web_view = null
    }
    if(window::window_is_created(&raw mut wv.win)) {
        window::window_destroy(&raw mut wv.win)
    }
}

public func webview_load_url(wv : *mut WebView, url : *char) {
    if(wv.web_view != null) {
        webkit_web_view_load_uri(wv.web_view as *mut WebKitWebView, url)
    }
}

public func webview_load_html(wv : *mut WebView, html : *char) {
    if(wv.web_view != null) {
        webkit_web_view_load_html(wv.web_view as *mut WebKitWebView, html, "about:blank\0" as *char)
    }
}

public func webview_evaluate_js(wv : *mut WebView, script : *char) {
    if(wv.web_view != null) {
        webkit_web_view_run_javascript(wv.web_view as *mut WebKitWebView, script, null, null, null)
        // Pump the GLib main loop until no more events are pending so the queued
        // JS (e.g. onReply) actually executes. A single iteration may pick up an
        // unrelated event (e.g. download-thread I/O) and leave the JS evaluation
        // callback still queued; the Promise would then never resolve.
        var safety = 0
        while(gtk_events_pending() != 0 && safety < 500) {
            gtk_main_iteration()
            safety = safety + 1
        }
    }
}

// Evaluate JavaScript and receive the result through an asynchronous callback.
// `result` is the JS value converted to a string (jsc_value_to_string — a JS
// number arrives as "42", a JS string as "hello") and is valid only during
// the callback call; copy it if needed. On evaluation failure `result` is
// null. The callback runs on the GLib main loop.
//
// NOTE: WebKitGTK fails script evaluation while a page is still loading, so
// call this AFTER the page has finished loading (e.g. from a g_timeout_add /
// load-completion callback), not immediately after webview_load_html.
func linux_js_callback(web_view : *mut WebKitWebView, res : *mut void, user_data : *mut void) {
    var ctx = user_data as *mut JsEvalHandler
    if(ctx == null || ctx.cb == null) {
        if(ctx != null) {
            free(ctx as *mut void)
        }
        return
    }
    var js_result = webkit_web_view_run_javascript_finish(web_view, res, null)
    if(js_result != null) {
        var value = webkit_javascript_result_get_js_value(js_result)
        if(value != null) {
            var s = jsc_value_to_string(value)
            if(s != null) {
                ctx.cb(ctx.user_data, s)
            } else {
                ctx.cb(ctx.user_data, null)
            }
        } else {
            ctx.cb(ctx.user_data, null)
        }
        webkit_javascript_result_unref(js_result)
    } else {
        ctx.cb(ctx.user_data, null)
    }
    free(ctx as *mut void)
}

public func webview_evaluate_js_result(
    wv : *mut WebView,
    script : *char,
    cb : JsResultCallback,
    user_data : *mut void
) {
    if(wv.web_view == null) {
        return
    }
    var ctx = malloc(sizeof(JsEvalHandler)) as *mut JsEvalHandler
    if(ctx == null) {
        return
    }
    ctx.cb = cb
    ctx.user_data = user_data
    webkit_web_view_run_javascript(
        wv.web_view as *mut WebKitWebView,
        script,
        null,
        linux_js_callback as *mut void,
        ctx as *mut void
    )
}

public func webview_title(wv : *mut WebView) : string {
    if(wv.web_view != null) {
        var title_ptr = webkit_web_view_get_title(wv.web_view as *mut WebKitWebView)
        if(title_ptr != null) {
            var result = string("")
            var i : size_t = 0
            while(title_ptr[i] != '\0' as char) {
                result.append(title_ptr[i])
                i += 1
            }
            return result
        }
    }
    // copy() — returning wv.title by value would be a shallow copy whose
    // destructor frees the same heap buffer as wv.title (double free).
    return wv.title.copy()
}

public func webview_set_title(wv : *mut WebView, title : *char) {
    wv.title = string("")
    wv.title.append_char_ptr(title)
    if(!wv.attached && window::window_is_created(&raw mut wv.win)) {
        window::window_set_title(&raw mut wv.win, title)
    }
}

public func webview_set_size(wv : *mut WebView, width : int, height : int) {
    wv.width = width
    wv.height = height
    if(!wv.attached && window::window_is_created(&raw mut wv.win)) {
        window::window_set_size(&raw mut wv.win, width, height)
    }
}

public func webview_show(wv : *mut WebView) {
    if(wv.attached) {
        // embed mode: the app owns the parent window; just make sure the
        // webview section itself is visible
        if(wv.fixed != null) {
            gtk_widget_show_all(wv.fixed)
        }
        wv.visible = true
    } else if(window::window_is_created(&raw mut wv.win)) {
        window::window_show(&raw mut wv.win)
        wv.visible = true
    }
}

public func webview_hide(wv : *mut WebView) {
    wv.visible = false
    if(wv.attached) {
        if(wv.fixed != null) {
            gtk_widget_hide(wv.fixed)
        }
    } else if(window::window_is_created(&raw mut wv.win)) {
        window::window_hide(&raw mut wv.win)
    }
}

public func webview_run(wv : *mut WebView) {
    if(!wv.attached) {
        window::window_run()
        if(window::window_quit_by_destroy() != 0) {
            // The user closed the window: the webview and window are gone.
            // Clear the stale references so webview_destroy is a no-op.
            wv.web_view = null
            wv.win.created = false
            wv.win.widget = null
            wv.win.visible = false
        }
    }
}

public func webview_stop(wv : *mut WebView) {
    window::window_quit()
}

// Access the webview's underlying window (from the window library). In
// standalone mode this is the top-level window the webview created; in embed
// mode it is the parent window the webview was attached to. Use it to mix
// native UI with the webview — add controls, handle callbacks, set
// title/size, etc. — all through the window library's API.
public func webview_window(wv : *mut WebView) : *mut window::Window {
    if(wv.attached && wv.parent_win != null) {
        return wv.parent_win
    }
    return &raw mut wv.win
}

// ===========================================================================
// New API features (Linux stubs — to be implemented)
// ===========================================================================

// Closure carried through the GLib idle source for webview_dispatch.
struct DispatchClosure {
    var fn : DispatchCallback
    var arg : *mut void
}

// GLib GSourceFunc: runs the closure on the GTK main loop (one-shot).
func linux_dispatch_idle(data : *mut void) : i32 {
    var c = data as *mut DispatchClosure
    if(c != null && c.fn != null) {
        c.fn(c.arg)
    }
    if(c != null) {
        free(c as *mut void)
    }
    return 0
}

// Schedule a function to run on the GTK main loop. Thread-safe.
public func webview_dispatch(wv : *mut WebView, fn : DispatchCallback, arg : *mut void) {
    if(fn == null) {
        return
    }
    var c = malloc(sizeof(DispatchClosure)) as *mut DispatchClosure
    if(c == null) {
        return
    }
    c.fn = fn
    c.arg = arg
    g_idle_add(linux_dispatch_idle as *mut void, c as *mut void)
}

// Set window size with a hint (none / min / max / fixed).
public func webview_set_size_hints(wv : *mut WebView, width : int, height : int, hint : int) {
    wv.width = width
    wv.height = height
    if(!wv.attached && window::window_is_created(&raw mut wv.win) && wv.web_view != null) {
        var win = window::window_native_handle(&raw mut wv.win) as *mut GtkWindow
        if(hint == webview::SIZE_HINT_FIXED) {
            gtk_widget_set_size_request(wv.web_view as *mut GtkWidget, width, height)
            gtk_window_set_resizable(win, 0)
        } else if(hint == webview::SIZE_HINT_MIN) {
            gtk_widget_set_size_request(wv.web_view as *mut GtkWidget, width, height)
            gtk_window_set_resizable(win, 1)
        } else if(hint == webview::SIZE_HINT_MAX) {
            gtk_window_set_default_size(win, width, height)
            gtk_window_set_resizable(win, 1)
        } else {
            // SIZE_HINT_NONE: just apply the size.
            gtk_window_set_default_size(win, width, height)
            gtk_window_set_resizable(win, 1)
        }
    }
}

// Inject JavaScript that runs on every page load.
public func webview_init(wv : *mut WebView, js : *char) {
    if(wv.web_view == null) {
        return
    }
    var manager = webkit_web_view_get_user_content_manager(wv.web_view as *mut WebKitWebView)
    if(manager == null) {
        return
    }
    var script = webkit_user_script_new(
        js,
        WEBKIT_USER_CONTENT_INJECT_TOP_FRAME,
        WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
        null,
        null)
    if(script != null) {
        webkit_user_content_manager_add_script(manager, script)
    }
}

// Remove a binding (single global handler on this backend).
public func webview_unbind(wv : *mut WebView, name : *char) {
    var was_bound = wv.bind_handler_set
    if(wv.bind_ctx != null) {
        delete wv.bind_ctx
        wv.bind_ctx = null
    }
    wv.bind_handler_set = false
    wv.bind_signal_connected = false
    if(wv.web_view != null && name != null && was_bound) {
        var js = string("window.__webview__.onUnbind('")
        var i = 0
        while(name[i] != '\0' as char) {
            js.append(name[i])
            i = i + 1
        }
        js.append_view(std::string_view::make_no_len("')"))
        webview_evaluate_js(wv, js.data())
    }
}

// Respond to an async binding call (used by handlers invoked from other threads).
public func webview_return(wv : *mut WebView, id : *char, status : int, result : *char) {
    if(wv.web_view == null) {
        return
    }
    var id_str = string("")
    if(id != null) {
        var i = 0
        while(id[i] != '\0' as char) { id_str.append(id[i]); i = i + 1 }
    }
    // Compute result size for chunking check
    var res_size : size_t = 0
    if(result != null) {
        var j = 0
        while(result[j] != '\0' as char) { j = j + 1 }
        res_size = j as size_t
    }
    if(res_size >= LARGE_RESULT_THRESHOLD && result != null) {
        linux_store_large_result(id_str.data(), result, res_size)
        var notify = string("window.__webview__._onLargeResult(")
        var nid_view = std::string_view::make_view(&id_str)
        var esc_nid = linux_json_escape(nid_view)
        var esc_nid_view = std::string_view::make_view(&esc_nid)
        notify.append_view(&esc_nid_view)
        notify.append_view(std::string_view::make_no_len(", "))
        notify.append_integer(res_size as bigint)
        notify.append(')')
        webview_evaluate_js(wv, notify.data())
        return
    }
    // Small result: send inline.
    var js_call = string("window.__webview__.onReply(")
    var esc_id = linux_json_escape(std::string_view::make_view(&id_str))
    var esc_id_view = std::string_view::make_view(&esc_id)
    js_call.append_view(&esc_id_view)
    js_call.append_view(std::string_view::make_no_len(", "))
    js_call.append_integer(status as bigint)
    js_call.append_view(std::string_view::make_no_len(", "))
    if(result != null) {
        var res_str = string("")
        var j2 = 0
        while(result[j2] != '\0' as char) { res_str.append(result[j2]); j2 = j2 + 1 }
        var esc_res = linux_json_escape(std::string_view::make_view(&res_str))
        var esc_res_view = std::string_view::make_view(&esc_res)
        js_call.append_view(&esc_res_view)
    } else {
        js_call.append_view(std::string_view::make_no_len("undefined"))
    }
    js_call.append(')')
    webview_evaluate_js(wv, js_call.data())
}

// Get a native handle by kind.
public func webview_get_native_handle(wv : *mut WebView, kind : int) : *mut void {
    if(wv.web_view == null) {
        return null
    }
    if(kind == webview::NATIVE_HANDLE_WINDOW) {
        return window::window_native_handle(&raw mut wv.win) as *mut void
    }
    if(kind == webview::NATIVE_HANDLE_WIDGET) {
        return wv.web_view as *mut void
    }
    if(kind == webview::NATIVE_HANDLE_BROWSER_CONTROLLER) {
        return wv.web_view as *mut void
    }
    return null
}

// Get the library version information.
public func webview_version() : WebViewVersion {
    return WebViewVersion { major : 0, minor : 12, patch : 1 }
}

} // end namespace webview
