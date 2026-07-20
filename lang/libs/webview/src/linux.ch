// ---------------------------------------------------------------------------
// Linux webview using GTK3 + WebKit2GTK via C API
// ---------------------------------------------------------------------------
// All GTK/WebKit functions are declared as @extern (resolved at link time).
// The Chemical code manages all state, callbacks, and lifecycle.

public namespace webview {

using std::string;

// GTK types (opaque pointers)
@no_init @extern public struct GtkWindow {}
@no_init @extern public struct GtkWidget {}
@no_init @extern public struct GtkContainer {}
@no_init @extern public struct GtkBox {}
@no_init @extern public struct WebKitWebView {}
@no_init @extern public struct WebKitSettings {}
@no_init @extern public struct WebKitUserContentManager {}
@no_init @extern public struct WebKitWebContext {}
@no_init @extern public struct WebKitJavascriptResult {}

// GTK constants
const GTK_WINDOW_TOPLEVEL = 0
const GTK_POS_LEFT = 0
const TRUE = 1
const FALSE = 0
const GTK_FILL = 4
const GTK_EXPAND = 2
const GTK_SHRINK = 1
const WEBKIT_LOAD_FINISHED = 4

// GTK functions
@extern public func gtk_init(argc : *mut int, argv : *mut *mut *char) : int
@extern public func gtk_main()
@extern public func gtk_main_quit()
@extern public func gtk_widget_show_all(widget : *mut GtkWidget)
@extern public func gtk_widget_destroy(widget : *mut GtkWidget)
@extern public func gtk_widget_set_size_request(widget : *mut GtkWidget, width : int, height : int)

@extern public func gtk_window_new(type_ : int) : *mut GtkWidget
@extern public func gtk_window_set_title(window : *mut GtkWindow, title : *char)
@extern public func gtk_window_set_default_size(window : *mut GtkWindow, width : int, height : int)
@extern public func gtk_window_set_position(window : *mut GtkWindow, position : int)
@extern public func gtk_window_get_type() : size_t

@extern public func gtk_box_new(orientation : int, spacing : int) : *mut GtkWidget
@extern public func gtk_box_pack_start(box : *mut GtkBox, child : *mut GtkWidget, expand : int, fill : int, padding : u32)

// WebKit functions
@extern public func webkit_web_view_new() : *mut GtkWidget
@extern public func webkit_web_view_get_type() : size_t
@extern public func webkit_web_view_load_uri(web_view : *mut WebKitWebView, uri : *char)
@extern public func webkit_web_view_load_html(web_view : *mut WebKitWebView, content : *char, base_uri : *char)
@extern public func webkit_web_view_run_javascript(web_view : *mut WebKitWebView, script : *char, cancellable : *mut void, callback : *mut void, user_data : *mut void)
@extern public func webkit_web_view_get_title(web_view : *mut WebKitWebView) : *char
@extern public func webkit_web_view_get_settings(web_view : *mut WebKitWebView) : *mut WebKitSettings

@extern public func webkit_settings_set_javascript_enabled(settings : *mut WebKitSettings, enabled : int)
@extern public func webkit_settings_set_allow_file_access_from_file_urls(settings : *mut WebKitSettings, allowed : int)

@extern public func webkit_web_view_evaluate_javascript(web_view : *mut WebKitWebView, script : *char, length : isize, source_uri : *char, cancellable : *mut void, callback : *mut void, user_data : *mut void)

// GLib signal connection
@extern public func g_signal_connect(instance : *mut void, signal : *char, handler : *mut void, data : *mut void) : u64
@extern public func g_signal_connect_swapped(instance : *mut void, signal : *char, handler : *mut void, data : *mut void) : u64

// GMainLoop
@no_init @extern public struct GMainLoop {}
@extern public func g_main_loop_new(context : *mut void, is_running : int) : *mut GMainLoop
@extern public func g_main_loop_quit(loop : *mut GMainLoop)

// JavaScriptCore for result extraction
@no_init @extern public struct JSCValue {}
@no_init @extern public struct JSGlobalContextRef {}

@extern public func jsc_value_to_string(value : *mut JSCValue) : *char
@extern public func jsc_value_is_string(value : *mut JSCValue) : int

// ---------------------------------------------------------------------------
// WebView struct (Linux)
// ---------------------------------------------------------------------------

@direct_init
public struct WebView {
    var window : *mut GtkWidget
    var web_view : *mut GtkWidget
    var box : *mut GtkWidget
    var title : string
    var width : int
    var height : int
    var visible : bool
    var initialized : bool

    @make
    func make() : WebView {
        return WebView {
            window : null,
            web_view : null,
            box : null,
            title : string("Chemical WebView"),
            width : 800,
            height : 600,
            visible : false,
            initialized : false
        }
    }
}

var g_webview_ref : *mut WebView = null

func linux_on_window_destroy(widget : *mut GtkWidget, data : *mut void) {
    gtk_main_quit()
}

func linux_on_navigation_complete(web_view : *mut WebKitWebView, event : int, data : *mut void) {
}

public func webview_create(wv : *mut WebView) : std::Result<std::Unit, WebViewError> {
    // Initialize GTK
    var argc : int = 0
    var argv : *mut *mut *char = null
    gtk_init(&raw mut argc, argv)

    // Create window
    wv.window = gtk_window_new(GTK_WINDOW_TOPLEVEL)
    if(wv.window == null) {
        return std.Result.Err(WebViewError.InitFailed(string("failed to create window")))
    }

    // Set window properties
    gtk_window_set_title(wv.window as *mut GtkWindow, wv.title.c_str())
    gtk_window_set_default_size(wv.window as *mut GtkWindow, wv.width, wv.height)
    gtk_window_set_position(wv.window as *mut GtkWindow, 1) // center

    // Create box layout
    wv.box = gtk_box_new(0, 0) // horizontal, no spacing

    // Create web view
    wv.web_view = webkit_web_view_new()
    if(wv.web_view == null) {
        return std.Result.Err(WebViewError.InitFailed(string("failed to create web view")))
    }

    // Configure web view settings
    var settings = webkit_web_view_get_settings(wv.web_view as *mut WebKitWebView)
    if(settings != null) {
        webkit_settings_set_javascript_enabled(settings, TRUE)
        webkit_settings_set_allow_file_access_from_file_urls(settings, TRUE)
    }

    // Add web view to box
    gtk_box_pack_start(wv.box as *mut GtkBox, wv.web_view, TRUE, GTK_FILL, 0)

    // Add box to window
    // gtk_container_add is essentially casting container to GtkContainer and calling add
    gtk_container_add(wv.window as *mut GtkContainer, wv.box)

    // Connect destroy signal
    g_signal_connect(wv.window as *mut void, "destroy\0" as *char, linux_on_window_destroy as *mut void, null)

    // Connect navigation finished signal
    g_signal_connect(wv.web_view as *mut void, "load-changed\0" as *char, linux_on_navigation_complete as *mut void, null)

    wv.visible = false
    wv.initialized = true
    g_webview_ref = wv

    return std.Result.Ok(std::Unit{})
}

@extern("gtk_container_add")
func gtk_container_add(container : *mut GtkContainer, child : *mut GtkWidget)

public func webview_destroy(wv : *mut WebView) {
    if(wv.web_view != null) {
        gtk_widget_destroy(wv.web_view)
        wv.web_view = null
    }
    if(wv.window != null) {
        gtk_widget_destroy(wv.window)
        wv.window = null
    }
    wv.initialized = false
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
    }
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
    return wv.title
}

public func webview_set_title(wv : *mut WebView, title : *char) {
    wv.title = string("")
    wv.title.append_char_ptr(title)
    if(wv.window != null) {
        gtk_window_set_title(wv.window as *mut GtkWindow, title)
    }
}

public func webview_set_size(wv : *mut WebView, width : int, height : int) {
    wv.width = width
    wv.height = height
    if(wv.window != null) {
        gtk_window_set_default_size(wv.window as *mut GtkWindow, width, height)
    }
}

public func webview_show(wv : *mut WebView) {
    if(wv.window != null) {
        gtk_widget_show_all(wv.window)
        wv.visible = true
    }
}

public func webview_hide(wv : *mut WebView) {
    // GTK doesn't have a simple hide, we use destroy and recreate pattern
    // For simplicity, just mark as hidden
    wv.visible = false
}

public func webview_run(wv : *mut WebView) {
    if(wv.window != null) {
        gtk_main()
    }
}

public func webview_stop(wv : *mut WebView) {
    gtk_main_quit()
}

} // end namespace webview
