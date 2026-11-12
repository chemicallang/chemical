// P2.5 — compile-only coverage for the safe window/webview handles.
//
// These functions are intentionally NOT annotated `@test`: creating a native
// window/webview needs a graphical display, so the `--webview` suite only
// compiles them. They type-check every method of `window::Window` and
// `webview::WebView` (including callback registration/unregistration and
// the JS bridge) without ever running.

using std::Option;
using std::string;
using std::string_view;

func p25_safe_window_api_typechecks() {
    var created = window::Window.new("P2.5 window", 640, 480)
    if(created is std::Result.Ok) {
        var Ok(w) = created else unsafe { unreachable }
        w.set_title("title")
        var _s = w.title()
        var _size = w.size()
        var _client = w.client_size()
        w.set_size(800, 600)
        var _pos = w.position()
        w.set_position(10, 20)
        w.show()
        w.hide()
        w.focus()
        w.set_cursor(window::CURSOR_HAND)
        var icon = w.set_icon("icon.png")
        if(icon is std::Result.Err) { }
        w.set_opacity(0.9)
        w.set_always_on_top(true)
        w.set_decorated(false)
        var _dpi = w.dpi()
        var _scale = w.scale_factor()
        var _monitors = w.monitor_count()
        var _bounds = w.monitor_bounds(0)
        var _mon_scale = w.monitor_scale(0)
        var _clip = w.get_clipboard()
        var _set_clip = w.set_clipboard("text")

        w.on_resize((width : int, height : int) => {})
        w.off_resize()
        w.on_close(() => {})
        w.off_close()
        w.on_focus((focused : bool) => {})
        w.off_focus()
        w.on_event((event : &window::Event) => {})
        w.off_event()
        w.on_drop((path : string_view) => {})
        w.off_drop()
        w.on_command((id : int) => {})
        w.off_command()

        var timer = w.set_timer(100, () => {})
        if(timer is std::Result.Ok) {
            var Ok(timer_id) = timer else unsafe { unreachable }
            w.cancel_timer(timer_id)
        }

        var _open = w.is_open()
        w.close()
    }
}

func p25_safe_webview_api_typechecks() {
    var created = webview::WebView.new("P2.5 webview", 800, 600)
    if(created is std::Result.Ok) {
        var Ok(w) = created else unsafe { unreachable }
        w.load_url("https://example.com")
        w.load_html("<p>hello</p>")
        w.evaluate("1 + 1")
        w.evaluate_result("1 + 1", (result : Option<string>) => {})
        var _title = w.title()
        w.set_title("title")
        w.set_size(1024, 768)
        w.show()
        w.hide()

        var bound = w.bind((method : string_view, args : string_view) => {
            return string("{}")
        })
        if(bound is std::Result.Err) { }
        w.unbind("method")
        w.return_value("id", 0, "{}")

        unsafe {
            w.dispatch((arg : *mut void) => {}, null)
            var _native = w.native_handle(webview::NATIVE_HANDLE_WINDOW)
        }

        w.stop()
        w.run()
    }
}
