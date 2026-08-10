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
@no_init @extern public struct GtkFixed {}
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

// GtkFixed: absolute-positioning container used by webview_attach to place the
// webview in a section of an existing app window/container
@extern public func gtk_fixed_new() : *mut GtkWidget
@extern public func gtk_fixed_put(fixed : *mut GtkFixed, widget : *mut GtkWidget, x : int, y : int)
@extern public func gtk_fixed_move(fixed : *mut GtkFixed, widget : *mut GtkWidget, x : int, y : int)
@extern public func gtk_widget_show(widget : *mut GtkWidget)
@extern public func gtk_widget_hide(widget : *mut GtkWidget)

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

// ---------------------------------------------------------------------------
// WebView struct (Linux)
// ---------------------------------------------------------------------------

@direct_init
public struct WebView {
    var window : *mut GtkWidget
    var web_view : *mut GtkWidget
    var box : *mut GtkWidget

    // --- embed mode (webview_attach): the webview lives in a section of an
    //     existing app window/container instead of its own top-level window.
    //     `fixed` is a GtkFixed (added to the app's container) that holds the
    //     webview at the section (bounds_x/y/w/h); native UI can live in the
    //     rest of the app's window. ---
    var attached : bool
    var parent_widget : *mut GtkWidget
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

    @make
    func make() : WebView {
        return WebView {
            window : null,
            web_view : null,
            box : null,
            attached : false,
            parent_widget : null,
            fixed : null,
            bounds_x : 0,
            bounds_y : 0,
            bounds_w : 0,
            bounds_h : 0,
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
    // (JavaScript is always enabled in modern WebKitGTK (>= 2.40);
    // webkit_settings_set_javascript_enabled no longer exists there)
    var settings = webkit_web_view_get_settings(wv.web_view as *mut WebKitWebView)
    if(settings != null) {
        webkit_settings_set_allow_file_access_from_file_urls(settings, TRUE)
    }

    // Add web view to box
    gtk_box_pack_start(wv.box as *mut GtkBox, wv.web_view, TRUE, GTK_FILL, 0)

    // Add box to window
    // gtk_container_add is essentially casting container to GtkContainer and calling add
    gtk_container_add(wv.window as *mut GtkContainer, wv.box)

    // Connect destroy signal
    g_signal_connect_data(wv.window as *mut void, "destroy\0" as *char, linux_on_window_destroy as *mut void, null, null, 0)

    // Connect navigation finished signal
    g_signal_connect_data(wv.web_view as *mut void, "load-changed\0" as *char, linux_on_navigation_complete as *mut void, null, null, 0)

    wv.visible = false
    wv.initialized = true
    g_webview_ref = wv

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

// Attach the webview to an app-owned GtkWidget (a window or any container)
// instead of creating its own top-level window. The webview occupies the
// section (x, y, width, height) inside a GtkFixed that is added to the app's
// container; native UI can live in the rest of the app's window. Move the
// section with webview_set_bounds. On Linux `parent` is a *mut GtkWidget; on
// Windows it is an HWND.
public func webview_attach(
    wv : *mut WebView,
    parent : *mut GtkWidget,
    x : int,
    y : int,
    width : int,
    height : int
) : std::Result<std::Unit, WebViewError> {
    if(parent == null) {
        return std.Result.Err(WebViewError.InitFailed(string("webview_attach: parent widget is null")))
    }
    if(wv.attached) {
        return std.Result.Err(WebViewError.InitFailed(string("webview_attach: webview is already attached")))
    }
    // NOTE: no gtk_init() here — the app must already have initialized GTK to
    // own the parent GtkWidget, and gtk_init may only be called once per
    // process (a second call logs a GTK warning).

    wv.attached = true
    wv.parent_widget = parent
    wv.bounds_x = x
    wv.bounds_y = y
    wv.bounds_w = width
    wv.bounds_h = height

    wv.web_view = webkit_web_view_new()
    if(wv.web_view == null) {
        wv.attached = false
        wv.parent_widget = null
        return std.Result.Err(WebViewError.InitFailed(string("failed to create web view")))
    }
    var settings = webkit_web_view_get_settings(wv.web_view as *mut WebKitWebView)
    if(settings != null) {
        webkit_settings_set_allow_file_access_from_file_urls(settings, TRUE)
    }

    // Absolute-position the webview inside a GtkFixed so it occupies its
    // section of the app's existing window/container.
    wv.fixed = gtk_fixed_new()
    gtk_widget_set_size_request(wv.web_view, width, height)
    gtk_fixed_put(wv.fixed as *mut GtkFixed, wv.web_view, x, y)
    gtk_container_add(parent as *mut GtkContainer, wv.fixed)
    gtk_widget_show_all(wv.fixed)

    wv.initialized = true
    return std.Result.Ok(std::Unit{})
}

// Move/resize the webview section inside the attached parent widget (in the
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
    if(wv.attached) {
        // embed mode: destroy only the webview + fixed container; the app owns
        // the parent window/container.
        if(wv.web_view != null) {
            gtk_widget_destroy(wv.web_view)
            wv.web_view = null
        }
        if(wv.fixed != null) {
            gtk_widget_destroy(wv.fixed)
            wv.fixed = null
        }
        wv.attached = false
        wv.parent_widget = null
        wv.initialized = false
        return
    }
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
    } else if(wv.attached && wv.fixed != null) {
        // embed mode: the app owns the parent window; just make sure the
        // webview section itself is visible
        gtk_widget_show_all(wv.fixed)
        wv.visible = true
    }
}

public func webview_hide(wv : *mut WebView) {
    // GTK doesn't have a simple hide on the window, we use destroy and
    // recreate pattern. In embed mode we can hide the webview section.
    wv.visible = false
    if(wv.attached && wv.fixed != null) {
        gtk_widget_hide(wv.fixed)
    }
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
