// ---------------------------------------------------------------------------
// linux.ch — Linux backend for the window library (GTK3 via C API).
//
// Same public API as win/win.ch. GTK owns the window lifecycle; this file
// keeps the same logical-size convention: window_set_size / window_create
// treat width/height as 96-dpi logical units (scaled by the monitor scale
// factor), and the resize callback reports physical pixels.
// ---------------------------------------------------------------------------

public namespace window {

using std::string;
using std::Option;
using std::string_view;

// 32-bit unsigned, matching GDK's guint32 event fields (cstd only defines
// DWORD on Windows; the Windows backend maps it to ulong there).
public type DWORD = u32

// ===========================================================================
// GTK / GDK types
// ===========================================================================

@no_init @extern public struct GtkWindow {}
@no_init @extern public struct GtkWidget {}
@no_init @extern public struct GtkContainer {}
@no_init @extern public struct GdkWindow {}
@no_init @extern public struct GdkDisplay {}
@no_init @extern public struct GdkMonitor {}
@no_init @extern public struct GdkCursor {}
@no_init @extern public struct GtkSelectionData {}
@no_init @extern public struct GError {}

public struct GtkAllocation {
    var x : int
    var y : int
    var width : int
    var height : int
}

public struct GdkRectangle {
    var x : int
    var y : int
    var width : int
    var height : int
}

// Mirror of the Windows backend's RECT (used by the shared Window struct's
// fullscreen-restore state; kept for layout parity across platforms).
public struct RECT {
    var left : int
    var top : int
    var right : int
    var bottom : int
}

// First four fields of GdkGeometry (used with GDK_HINT_MIN_SIZE/MAX_SIZE).
public struct GdkGeometry {
    var min_width : int
    var min_height : int
    var max_width : int
    var max_height : int
}

// GdkEventKey (GTK3 layout — field order/offsets must match gdkevents.h).
public struct GdkEventKey {
    var type_ : int
    var pad0 : int
    var window : *mut GdkWindow
    var send_event : i8
    var pad1 : int
    var time : DWORD
    var state : DWORD
    var keyval : DWORD
    var length : int
    var string : *char
    var hardware_keycode : ushort
    var group : uchar
    var pad2 : int
}

// GdkEventButton (x/y are doubles).
public struct GdkEventButton {
    var type_ : int
    var pad0 : int
    var window : *mut GdkWindow
    var send_event : i8
    var pad1 : int
    var time : DWORD
    var x : double
    var y : double
    var axes : *mut double
    var state : DWORD
    var button : DWORD
    var device : *mut void
    var x_root : double
    var y_root : double
}

// GdkEventMotion.
public struct GdkEventMotion {
    var type_ : int
    var pad0 : int
    var window : *mut GdkWindow
    var send_event : i8
    var pad1 : int
    var time : DWORD
    var x : double
    var y : double
    var axes : *mut double
    var state : DWORD
    var is_hint : i16
    var device : *mut void
    var x_root : double
    var y_root : double
}

// GdkEventScroll.
public struct GdkEventScroll {
    var type_ : int
    var pad0 : int
    var window : *mut GdkWindow
    var send_event : i8
    var pad1 : int
    var time : DWORD
    var x : double
    var y : double
    var state : DWORD
    var direction : int
    var device : *mut void
    var x_root : double
    var y_root : double
    var delta_x : double
    var delta_y : double
}

// ===========================================================================
// GTK / GDK constants
// ===========================================================================

const GTK_WINDOW_TOPLEVEL = 0
const GTK_WIN_POS_CENTER = 1
const TRUE = 1
const FALSE = 0

// event masks (gtk_widget_add_events)
public comptime const GDK_KEY_PRESS_MASK : int = 1 << 0
public comptime const GDK_KEY_RELEASE_MASK : int = 1 << 1
public comptime const GDK_POINTER_MOTION_MASK : int = 1 << 6
public comptime const GDK_BUTTON_PRESS_MASK : int = 1 << 8
public comptime const GDK_BUTTON_RELEASE_MASK : int = 1 << 9
public comptime const GDK_SCROLL_MASK : int = 1 << 12

// geometry hints
public comptime const GDK_HINT_MIN_SIZE : int = 2 // 1 << 1
public comptime const GDK_HINT_MAX_SIZE : int = 4 // 1 << 2

// drag & drop
const GTK_DEST_DEFAULT_ALL = 7
const GDK_ACTION_COPY = 2

// ===========================================================================
// GTK / GDK functions
// ===========================================================================

@extern public func gtk_init(argc : *mut int, argv : *mut *mut *char) : int
@extern public func gtk_init_check(argc : *mut int, argv : *mut *mut *char) : int
@extern public func gtk_main()
@extern public func gtk_main_quit()

@extern public func gtk_window_new(type_ : int) : *mut GtkWidget
@extern public func gtk_window_set_title(window : *mut GtkWindow, title : *char)
@extern public func gtk_window_set_default_size(window : *mut GtkWindow, width : int, height : int)
@extern public func gtk_window_set_position(window : *mut GtkWindow, position : int)
@extern public func gtk_window_move(window : *mut GtkWindow, x : int, y : int)
@extern public func gtk_window_get_position(window : *mut GtkWindow, x : *mut int, y : *mut int)
@extern public func gtk_window_get_size(window : *mut GtkWindow, width : *mut int, height : *mut int)
@extern public func gtk_window_maximize(window : *mut GtkWindow)
@extern public func gtk_window_unmaximize(window : *mut GtkWindow)
@extern public func gtk_window_iconify(window : *mut GtkWindow)
@extern public func gtk_window_deiconify(window : *mut GtkWindow)
@extern public func gtk_window_fullscreen(window : *mut GtkWindow)
@extern public func gtk_window_unfullscreen(window : *mut GtkWindow)
@extern public func gtk_window_set_decorated(window : *mut GtkWindow, setting : int)
@extern public func gtk_window_set_opacity(window : *mut GtkWindow, opacity : double)
@extern public func gtk_window_set_keep_above(window : *mut GtkWindow, setting : int)
@extern public func gtk_window_set_icon_from_file(window : *mut GtkWindow, filename : *char, error : *mut *mut GError) : int
@extern public func gtk_window_set_geometry_hints(window : *mut GtkWindow, geometry_widget : *mut GtkWidget, geometry : *mut GdkGeometry, flags : int)
@extern public func gtk_window_present(window : *mut GtkWindow)

@extern public func gtk_widget_show(widget : *mut GtkWidget)
@extern public func gtk_widget_show_all(widget : *mut GtkWidget)
@extern public func gtk_widget_hide(widget : *mut GtkWidget)
@extern public func gtk_widget_destroy(widget : *mut GtkWidget)
@extern public func gtk_widget_get_window(widget : *mut GtkWidget) : *mut GdkWindow
@extern public func gtk_widget_add_events(widget : *mut GtkWidget, events : int)

@extern public func g_signal_connect_data(instance : *mut void, signal : *char, handler : *mut void, data : *mut void, destroy_data : *mut void, connect_flags : int) : u64
@extern public func g_idle_add(function : *mut void, data : *mut void) : u32

// GObject object-data — used for the stable per-widget callback context
// (g_object_set_data_full auto-frees the context when the widget is destroyed)
@extern public func g_object_set_data_full(object : *mut void, key : *char, data : *mut void, destroy : *mut void)
@extern public func g_object_get_data(object : *mut void, key : *char) : *mut void

// GDK
@extern public func gdk_display_get_default() : *mut GdkDisplay
@extern public func gdk_display_get_n_monitors(display : *mut GdkDisplay) : int
@extern public func gdk_display_get_monitor(display : *mut GdkDisplay, num : int) : *mut GdkMonitor
@extern public func gdk_display_get_monitor_at_window(display : *mut GdkDisplay, win : *mut GdkWindow) : *mut GdkMonitor
@extern public func gdk_monitor_get_geometry(monitor : *mut GdkMonitor, rect : *mut GdkRectangle)
@extern public func gdk_monitor_get_scale_factor(monitor : *mut GdkMonitor) : int
@extern public func gdk_window_set_cursor(window : *mut GdkWindow, cursor : *mut GdkCursor)
@extern public func gdk_cursor_new_from_name(display : *mut GdkDisplay, name : *char) : *mut GdkCursor

// drag & drop
@extern public func gtk_drag_dest_set(widget : *mut GtkWidget, flags : int, targets : *mut void, n_targets : int, actions : int)
@extern public func gtk_selection_data_get_uris(data : *mut GtkSelectionData) : *mut *mut char
@extern public func g_filename_from_uri(uri : *char, hostname : *mut *mut char, error : *mut *mut GError) : *char
@extern public func g_free(mem : *mut void)

// ===========================================================================
// the Window struct
// ===========================================================================

@direct_init
public struct Window {
    // native handle (null until created)
    var widget : *mut GtkWidget

    // logical geometry
    var title : string
    var width : int
    var height : int
    var x : int
    var y : int

    // constraints (0 = unconstrained)
    var min_w : int
    var min_h : int
    var max_w : int
    var max_h : int

    // state
    var fullscreen : bool
    var visible : bool
    var decorated : bool
    var always_on_top : bool
    var opacity : double
    var cursor : int
    var dpi : int
    var created : bool

    // fullscreen restore state
    var saved_style : LONG_PTR
    var saved_rect : RECT

    // callbacks + user data
    var user_data : *mut void
    var resize_cb : ResizeCallback
    var close_cb : CloseCallback
    var focus_cb : FocusCallback
    var event_cb : EventCallback
    var drop_cb : DropCallback
    var command_cb : CommandCallback

    @make
    func make() : Window {
        return Window {
            widget : null,
            title : string(""),
            width : 800,
            height : 600,
            x : 0,
            y : 0,
            min_w : 0,
            min_h : 0,
            max_w : 0,
            max_h : 0,
            fullscreen : false,
            visible : false,
            decorated : true,
            always_on_top : false,
            opacity : 1.0,
            cursor : CURSOR_ARROW,
            dpi : 96,
            created : false,
            saved_style : 0,
            saved_rect : RECT { left : 0, top : 0, right : 0, bottom : 0 },
            user_data : null,
            resize_cb : null,
            close_cb : null,
            focus_cb : null,
            event_cb : null,
            drop_cb : null,
            command_cb : null
        }
    }
}

// ===========================================================================
// helpers
// ===========================================================================

func cursor_name_for_kind(kind : int) : *char {
    switch(kind) {
        CURSOR_IBEAM => return "text"
        CURSOR_WAIT => return "wait"
        CURSOR_CROSS => return "crosshair"
        CURSOR_HAND => return "hand2"
        CURSOR_HELP => return "help"
        CURSOR_MOVE => return "move"
        CURSOR_RESIZE_NS => return "ns-resize"
        CURSOR_RESIZE_EW => return "ew-resize"
        CURSOR_RESIZE_NESW => return "nesw-resize"
        CURSOR_RESIZE_NWSE => return "nwse-resize"
        CURSOR_NO => return "no"
        CURSOR_APPSTARTING => return "progress"
        CURSOR_SIZEALL => return "fleur"
        default => return "default"
    }
}

func apply_cursor(w : *mut Window) {
    if(w.widget == null) {
        return
    }
    var gdkwin = gtk_widget_get_window(w.widget)
    if(gdkwin == null) {
        return
    }
    ensure_gtk_init()
    var cursor = gdk_cursor_new_from_name(gdk_display_get_default(), cursor_name_for_kind(w.cursor))
    if(cursor != null) {
        gdk_window_set_cursor(gdkwin, cursor)
    }
}

// ===========================================================================
// signal handlers
// ===========================================================================

// ---------------------------------------------------------------------------
// Stable per-widget callback context.
//
// GTK signal handlers must NOT capture the caller's Window* as user data:
// the webview library embeds a window::Window inside its WebView and returns
// the WebView by value, so the embedded Window can be relocated after
// window_create() returns — leaving signal handlers pointing at dead stack
// memory (SIGSEGV on the first emitted signal).
//
// Instead, each window_create() allocates a WindowCtx on the heap, stores the
// current callbacks there, and attaches it to the GtkWidget via
// g_object_set_data_full (auto-freed when the widget is destroyed). Handlers
// look the context up by widget, so they always read the live callback set.
// ---------------------------------------------------------------------------

@direct_init
public struct WindowCtx {
    // The context is attached to the widget via g_object_set_data_full (keyed
    // on the GtkWidget), so handlers look it up by widget and never need to
    // dereference a Window* that may have been relocated by a by-value move.
    var window : *mut Window
    var user_data : *mut void
    var resize_cb : ResizeCallback
    var close_cb : CloseCallback
    var focus_cb : FocusCallback
    var event_cb : EventCallback
    var drop_cb : DropCallback
    var command_cb : CommandCallback

    @make
    func make() : WindowCtx {
        return WindowCtx {
            window : null,
            user_data : null,
            resize_cb : null,
            close_cb : null,
            focus_cb : null,
            event_cb : null,
            drop_cb : null,
            command_cb : null
        }
    }
}

// Declared type-only (no initializer): the LLVM backend crashes on an empty
// array literal at module scope (ArrayValue::element_type() returns null for
// `[]`), while a type-only global takes the safe zero-init path in both
// backends. The buffer is filled by win_ctx_key() on first use.
var g_win_ctx_key : [32]char
var g_win_ctx_key_ready : int = 0

// g_object_set_data_full stores the key POINTER, not a copy, so the key
// string must outlive the widget — keep it in a module-level buffer.
func win_ctx_key() : *char {
    if(g_win_ctx_key_ready == 0) {
        var i : int = 0
        var src = "chem_window_ctx\0" as *char
        while(src[i] != '\0' as char && i < 31) {
            g_win_ctx_key[i] = src[i]
            i += 1
        }
        g_win_ctx_key[i] = '\0' as char
        g_win_ctx_key_ready = 1
    }
    return &raw g_win_ctx_key[0]
}

func win_ctx_free(data : *mut void) {
    if(data != null) {
        free(data as *mut void)
    }
}

func window_ctx_get(widget : *mut GtkWidget) : *mut WindowCtx {
    return g_object_get_data(widget as *mut void, win_ctx_key()) as *mut WindowCtx
}

// Sync the caller's Window callback fields into the stable context (called by
// every window_set_*_callback / window_set_user_data).
func window_ctx_sync(w : *mut Window) {
    if(w == null || w.widget == null) {
        return
    }
    var ctx = window_ctx_get(w.widget)
    if(ctx == null) {
        return
    }
    ctx.window = w
    ctx.user_data = w.user_data
    ctx.resize_cb = w.resize_cb
    ctx.close_cb = w.close_cb
    ctx.focus_cb = w.focus_cb
    ctx.event_cb = w.event_cb
    ctx.drop_cb = w.drop_cb
    ctx.command_cb = w.command_cb
}

// Set while gtk_main() is running (window_run). The destroy handler only
// quits the loop when the loop is actually active — otherwise destroying a
// window after window_run returned would fire a spurious gtk_main_quit
// (Gtk-CRITICAL: main_loops != NULL).
var g_in_main_loop : int = 0

// Set when the main loop exits because a window was destroyed (user closed
// it) rather than via window_quit(). The window library's main loop is global
// (window_run takes no window), so a single flag is the correct granularity.
// The webview library uses it to avoid destroying widgets that were already
// torn down together with their window.
var g_quit_by_destroy : int = 0

func linux_on_destroy(widget : *mut GtkWidget, data : *mut void) {
    // NOTE: never write through ctx.window here. webview::create() embeds a
    // window::Window in its WebView and returns it by value, relocating the
    // struct after window_create() — a context captured during create() then
    // points at stack memory that has since been reused, and writing to it
    // corrupts live data. State is cleared by the caller's own teardown:
    // window_destroy() / webview_destroy() are guarded by g_quit_by_destroy
    // and webview_run() clears the moved copy after the loop exits.
    //
    // g_quit_by_destroy is only set when the destroy happens DURING the main
    // loop (the user closed the window, so the loop exits because of it). A
    // direct window_destroy()/window_close() called between runs must not
    // mark the flag — otherwise a later window_destroy() on a still-alive
    // window would wrongly skip destroying its widget.
    if(g_in_main_loop != 0) {
        g_quit_by_destroy = 1
        gtk_main_quit()
    }
}

func linux_on_size_allocate(widget : *mut GtkWidget, alloc : *mut GtkAllocation, data : *mut void) {
    var ctx = window_ctx_get(widget)
    if(ctx != null && ctx.resize_cb != null) {
        ctx.resize_cb(ctx.user_data, alloc.width, alloc.height)
    }
}

func linux_on_delete(widget : *mut GtkWidget, event : *mut void, data : *mut void) : int {
    var ctx = window_ctx_get(widget)
    if(ctx != null && ctx.close_cb != null) {
        ctx.close_cb(ctx.user_data)
    }
    return 0 // allow the window to close
}

func linux_on_focus_in(widget : *mut GtkWidget, event : *mut void, data : *mut void) : int {
    var ctx = window_ctx_get(widget)
    if(ctx != null && ctx.focus_cb != null) {
        ctx.focus_cb(ctx.user_data, true)
    }
    return 0
}

func linux_on_focus_out(widget : *mut GtkWidget, event : *mut void, data : *mut void) : int {
    var ctx = window_ctx_get(widget)
    if(ctx != null && ctx.focus_cb != null) {
        ctx.focus_cb(ctx.user_data, false)
    }
    return 0
}

func deliver_key_event(ctx : *mut WindowCtx, kind : int, event : *mut GdkEventKey) {
    if(ctx == null || ctx.event_cb == null) {
        return
    }
    var ev = Event.make()
    ev.kind = kind
    ev.key = event.keyval as int
    ev.modifiers = 0
    if((event.state & 0x1) != 0) { ev.modifiers |= MOD_SHIFT } // GDK_SHIFT_MASK
    if((event.state & 0x4) != 0) { ev.modifiers |= MOD_CTRL }  // GDK_CONTROL_MASK
    if((event.state & 0x8) != 0) { ev.modifiers |= MOD_ALT }   // GDK_MOD1_MASK
    ctx.event_cb(ctx.user_data, &raw mut ev)
}

func linux_on_key_press(widget : *mut GtkWidget, event : *mut GdkEventKey, data : *mut void) : int {
    deliver_key_event(window_ctx_get(widget), EVENT_KEY_DOWN, event)
    return 0
}

func linux_on_key_release(widget : *mut GtkWidget, event : *mut GdkEventKey, data : *mut void) : int {
    deliver_key_event(window_ctx_get(widget), EVENT_KEY_UP, event)
    return 0
}

func deliver_mouse_event(ctx : *mut WindowCtx, kind : int, x : double, y : double, button : int, state : DWORD) {
    if(ctx == null || ctx.event_cb == null) {
        return
    }
    var ev = Event.make()
    ev.kind = kind
    ev.x = x as int
    ev.y = y as int
    ev.button = button
    ev.modifiers = 0
    if((state & 0x1) != 0) { ev.modifiers |= MOD_SHIFT }
    if((state & 0x4) != 0) { ev.modifiers |= MOD_CTRL }
    if((state & 0x8) != 0) { ev.modifiers |= MOD_ALT }
    ctx.event_cb(ctx.user_data, &raw mut ev)
}

func linux_on_button_press(widget : *mut GtkWidget, event : *mut GdkEventButton, data : *mut void) : int {
    var ctx = window_ctx_get(widget)
    if(ctx != null) {
        var button = MOUSE_LEFT
        if(event.button == 3) {
            button = MOUSE_RIGHT
        } else if(event.button == 2) {
            button = MOUSE_MIDDLE
        }
        deliver_mouse_event(ctx, EVENT_MOUSE_DOWN, event.x, event.y, button, event.state)
    }
    return 0
}

func linux_on_button_release(widget : *mut GtkWidget, event : *mut GdkEventButton, data : *mut void) : int {
    var ctx = window_ctx_get(widget)
    if(ctx != null) {
        var button = MOUSE_LEFT
        if(event.button == 3) {
            button = MOUSE_RIGHT
        } else if(event.button == 2) {
            button = MOUSE_MIDDLE
        }
        deliver_mouse_event(ctx, EVENT_MOUSE_UP, event.x, event.y, button, event.state)
    }
    return 0
}

func linux_on_motion(widget : *mut GtkWidget, event : *mut GdkEventMotion, data : *mut void) : int {
    var ctx = window_ctx_get(widget)
    if(ctx != null) {
        deliver_mouse_event(ctx, EVENT_MOUSE_MOVE, event.x, event.y, MOUSE_LEFT, event.state)
    }
    return 0
}

func linux_on_scroll(widget : *mut GtkWidget, event : *mut GdkEventScroll, data : *mut void) : int {
    var ctx = window_ctx_get(widget)
    if(ctx != null && ctx.event_cb != null) {
        var ev = Event.make()
        ev.kind = EVENT_MOUSE_WHEEL
        ev.x = event.x as int
        ev.y = event.y as int
        ev.y = (event.delta_y * 100.0) as int // wheel delta (approx.)
        ctx.event_cb(ctx.user_data, &raw mut ev)
    }
    return 0
}

// ---- drag & drop ----

func linux_on_drag_data_received(
    widget : *mut GtkWidget,
    context : *mut void,
    x : int,
    y : int,
    data : *mut GtkSelectionData,
    info : DWORD,
    time : DWORD,
    user_data : *mut void
) {
    var ctx = window_ctx_get(widget)
    if(ctx == null || ctx.drop_cb == null || data == null) {
        return
    }
    var uris = gtk_selection_data_get_uris(data)
    if(uris != null && *uris != null) {
        var err : *mut GError = null
        var path = g_filename_from_uri(*uris, null, &raw mut err)
        if(path != null) {
            ctx.drop_cb(ctx.user_data, path)
            g_free(path as *mut void)
        }
    }
}

// ===========================================================================
// public API
// ===========================================================================

public func window_create(w : *mut Window) : std::Result<std::Unit, WindowError> {
    if(w.created) {
        return std.Result.Err(WindowError.InvalidState(string("window_create: window already created")))
    }

    var argc : int = 0
    var argv : *mut *mut *char = null
    gtk_init(&raw mut argc, argv)

    w.widget = gtk_window_new(GTK_WINDOW_TOPLEVEL)
    if(w.widget == null) {
        return std.Result.Err(WindowError.CreateFailed(string("gtk_window_new failed")))
    }

    // scale logical -> physical via the monitor scale factor
    var display = gdk_display_get_default()
    var monitor = gdk_display_get_monitor(display, 0)
    var scale = 1
    if(monitor != null) {
        scale = gdk_monitor_get_scale_factor(monitor)
    }
    if(scale <= 0) {
        scale = 1
    }
    w.dpi = scale * 96

    gtk_window_set_title(w.widget as *mut GtkWindow, w.title.data())
    gtk_window_set_default_size(w.widget as *mut GtkWindow, (w.width * scale), (w.height * scale))
    gtk_window_set_position(w.widget as *mut GtkWindow, GTK_WIN_POS_CENTER)
    if(!w.decorated) {
        gtk_window_set_decorated(w.widget as *mut GtkWindow, FALSE)
    }
    if(w.always_on_top) {
        gtk_window_set_keep_above(w.widget as *mut GtkWindow, TRUE)
    }
    if(w.opacity < 1.0) {
        gtk_window_set_opacity(w.widget as *mut GtkWindow, w.opacity)
    }

    // enable input events
    gtk_widget_add_events(
        w.widget,
        GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK | GDK_POINTER_MOTION_MASK |
        GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_SCROLL_MASK
    )

    // min/max constraints
    if(w.min_w > 0 || w.min_h > 0 || w.max_w > 0 || w.max_h > 0) {
        var geom = GdkGeometry { min_width : w.min_w, min_height : w.min_h, max_width : w.max_w, max_height : w.max_h }
        var flags = 0
        if(w.min_w > 0 || w.min_h > 0) {
            flags |= GDK_HINT_MIN_SIZE
        }
        if(w.max_w > 0 || w.max_h > 0) {
            flags |= GDK_HINT_MAX_SIZE
        }
        gtk_window_set_geometry_hints(w.widget as *mut GtkWindow, null, &raw mut geom, flags)
    }

    // stable per-widget callback context: handlers look the live Window up by
    // widget, so they survive the Window struct being relocated (e.g. by-value
    // WebView moves in webview::create). Freed automatically at widget
    // finalize via g_object_set_data_full.
    var ctx = malloc(sizeof(WindowCtx)) as *mut WindowCtx
    if(ctx == null) {
        gtk_widget_destroy(w.widget)
        w.widget = null
        return std.Result.Err(WindowError.CreateFailed(string("out of memory allocating window context")))
    }
    ctx.window = w
    ctx.user_data = w.user_data
    ctx.resize_cb = w.resize_cb
    ctx.close_cb = w.close_cb
    ctx.focus_cb = w.focus_cb
    ctx.event_cb = w.event_cb
    ctx.drop_cb = w.drop_cb
    ctx.command_cb = w.command_cb
    g_object_set_data_full(w.widget as *mut void, win_ctx_key(), ctx as *mut void, win_ctx_free as *mut void)

    // signals
    g_signal_connect_data(w.widget as *mut void, "destroy\0" as *char, linux_on_destroy as *mut void, ctx as *mut void, null, 0)
    g_signal_connect_data(w.widget as *mut void, "size-allocate\0" as *char, linux_on_size_allocate as *mut void, w as *mut void, null, 0)
    g_signal_connect_data(w.widget as *mut void, "delete-event\0" as *char, linux_on_delete as *mut void, w as *mut void, null, 0)
    g_signal_connect_data(w.widget as *mut void, "focus-in-event\0" as *char, linux_on_focus_in as *mut void, w as *mut void, null, 0)
    g_signal_connect_data(w.widget as *mut void, "focus-out-event\0" as *char, linux_on_focus_out as *mut void, w as *mut void, null, 0)
    g_signal_connect_data(w.widget as *mut void, "key-press-event\0" as *char, linux_on_key_press as *mut void, w as *mut void, null, 0)
    g_signal_connect_data(w.widget as *mut void, "key-release-event\0" as *char, linux_on_key_release as *mut void, w as *mut void, null, 0)
    g_signal_connect_data(w.widget as *mut void, "button-press-event\0" as *char, linux_on_button_press as *mut void, w as *mut void, null, 0)
    g_signal_connect_data(w.widget as *mut void, "button-release-event\0" as *char, linux_on_button_release as *mut void, w as *mut void, null, 0)
    g_signal_connect_data(w.widget as *mut void, "motion-notify-event\0" as *char, linux_on_motion as *mut void, w as *mut void, null, 0)
    g_signal_connect_data(w.widget as *mut void, "scroll-event\0" as *char, linux_on_scroll as *mut void, w as *mut void, null, 0)
    g_signal_connect_data(w.widget as *mut void, "drag-data-received\0" as *char, linux_on_drag_data_received as *mut void, w as *mut void, null, 0)

    // accept file drops
    gtk_drag_dest_set(w.widget, GTK_DEST_DEFAULT_ALL, null, 0, GDK_ACTION_COPY)

    w.created = true
    return std.Result.Ok(std::Unit{})
}

public func window_destroy(w : *mut Window) {
    if(g_quit_by_destroy != 0) {
        // The window was already destroyed by the user (the main loop quit
        // because the destroy handler fired) — the widget pointer may be
        // dangling, so do not dereference it. Just clear the state.
        w.created = false
        w.widget = null
        w.visible = false
        return
    }
    if(w.widget != null) {
        gtk_widget_destroy(w.widget)
        w.widget = null
    }
    w.created = false
    w.visible = false
}

public func window_is_created(w : *mut Window) : bool {
    return w.created && w.widget != null
}

public func window_title(w : *mut Window) : string {
    // copy() — returning w.title by value would be a shallow copy whose
    // destructor frees the same heap buffer as w.title (double free).
    return w.title.copy()
}

public func window_set_title(w : *mut Window, title : *char) {
    w.title = string("")
    w.title.append_char_ptr(title)
    if(w.widget != null) {
        gtk_window_set_title(w.widget as *mut GtkWindow, title)
    }
}

public func window_size(w : *mut Window) : Size {
    return Size.make(w.width, w.height)
}

public func window_client_size(w : *mut Window) : Size {
    if(w.widget != null) {
        var width : int = 0
        var height : int = 0
        gtk_window_get_size(w.widget as *mut GtkWindow, &raw mut width, &raw mut height)
        return Size.make(width, height)
    }
    return Size.make(w.width, w.height)
}

public func window_set_size(w : *mut Window, width : int, height : int) {
    w.width = width
    w.height = height
    if(w.widget == null) {
        return
    }
    var scale = (w.dpi / 96)
    if(scale <= 0) {
        scale = 1
    }
    gtk_window_set_default_size(w.widget as *mut GtkWindow, width * scale, height * scale)
}

public func window_position(w : *mut Window) : Position {
    if(w.widget != null) {
        var x : int = 0
        var y : int = 0
        gtk_window_get_position(w.widget as *mut GtkWindow, &raw mut x, &raw mut y)
        return Position.make(x, y)
    }
    return Position.make(w.x, w.y)
}

public func window_set_position(w : *mut Window, x : int, y : int) {
    w.x = x
    w.y = y
    if(w.widget != null) {
        gtk_window_move(w.widget as *mut GtkWindow, x, y)
    }
}

public func window_min_size(w : *mut Window) : Size {
    return Size.make(w.min_w, w.min_h)
}

public func window_max_size(w : *mut Window) : Size {
    return Size.make(w.max_w, w.max_h)
}

public func window_set_min_size(w : *mut Window, width : int, height : int) {
    w.min_w = width
    w.min_h = height
    if(w.widget != null) {
        var geom = GdkGeometry { min_width : width, min_height : height, max_width : w.max_w, max_height : w.max_h }
        var flags = GDK_HINT_MIN_SIZE
        if(w.max_w > 0 || w.max_h > 0) {
            flags |= GDK_HINT_MAX_SIZE
        }
        gtk_window_set_geometry_hints(w.widget as *mut GtkWindow, null, &raw mut geom, flags)
    }
}

public func window_set_max_size(w : *mut Window, width : int, height : int) {
    w.max_w = width
    w.max_h = height
    if(w.widget != null) {
        var geom = GdkGeometry { min_width : w.min_w, min_height : w.min_h, max_width : width, max_height : height }
        var flags = GDK_HINT_MAX_SIZE
        if(w.min_w > 0 || w.min_h > 0) {
            flags |= GDK_HINT_MIN_SIZE
        }
        gtk_window_set_geometry_hints(w.widget as *mut GtkWindow, null, &raw mut geom, flags)
    }
}

public func window_set_fullscreen(w : *mut Window, fullscreen : bool) {
    if(w.widget == null) {
        return
    }
    if(fullscreen) {
        gtk_window_fullscreen(w.widget as *mut GtkWindow)
    } else {
        gtk_window_unfullscreen(w.widget as *mut GtkWindow)
    }
    w.fullscreen = fullscreen
}

public func window_is_fullscreen(w : *mut Window) : bool {
    return w.fullscreen
}

public func window_maximize(w : *mut Window) {
    if(w.widget != null) {
        gtk_window_maximize(w.widget as *mut GtkWindow)
    }
}

public func window_minimize(w : *mut Window) {
    if(w.widget != null) {
        gtk_window_iconify(w.widget as *mut GtkWindow)
    }
}

public func window_restore(w : *mut Window) {
    if(w.widget != null) {
        gtk_window_unmaximize(w.widget as *mut GtkWindow)
        gtk_window_deiconify(w.widget as *mut GtkWindow)
    }
}

public func window_show(w : *mut Window) {
    if(w.widget != null) {
        gtk_widget_show_all(w.widget)
    }
    w.visible = true
}

public func window_hide(w : *mut Window) {
    if(w.widget != null) {
        gtk_widget_hide(w.widget)
    }
    w.visible = false
}

public func window_focus(w : *mut Window) {
    if(w.widget != null) {
        gtk_window_present(w.widget as *mut GtkWindow)
    }
}

public func window_close(w : *mut Window) {
    if(w.widget != null) {
        gtk_widget_destroy(w.widget)
    }
}

public func window_set_cursor(w : *mut Window, cursor : int) {
    w.cursor = cursor
    apply_cursor(w)
}

public func window_set_icon(w : *mut Window, path : *char) {
    if(w.widget == null) {
        return
    }
    var err : *mut GError = null
    gtk_window_set_icon_from_file(w.widget as *mut GtkWindow, path, &raw mut err)
}

public func window_set_opacity(w : *mut Window, opacity : double) {
    w.opacity = opacity
    if(w.widget != null) {
        gtk_window_set_opacity(w.widget as *mut GtkWindow, opacity)
    }
}

public func window_set_always_on_top(w : *mut Window, on_top : bool) {
    w.always_on_top = on_top
    if(w.widget != null) {
        if(on_top) {
            gtk_window_set_keep_above(w.widget as *mut GtkWindow, TRUE)
        } else {
            gtk_window_set_keep_above(w.widget as *mut GtkWindow, FALSE)
        }
    }
}

public func window_set_decorated(w : *mut Window, decorated : bool) {
    w.decorated = decorated
    if(w.widget != null) {
        if(decorated) {
            gtk_window_set_decorated(w.widget as *mut GtkWindow, TRUE)
        } else {
            gtk_window_set_decorated(w.widget as *mut GtkWindow, FALSE)
        }
    }
}

public func window_dpi(w : *mut Window) : int {
    return w.dpi
}

public func window_scale_factor(w : *mut Window) : double {
    return (w.dpi as double) / 96.0
}

public func window_monitor_count() : int {
    ensure_gtk_init()
    return gdk_display_get_n_monitors(gdk_display_get_default())
}

public func window_monitor_bounds(index : int) : Rect {
    ensure_gtk_init()
    var display = gdk_display_get_default()
    var monitor = gdk_display_get_monitor(display, index)
    var rect = GdkRectangle { x : 0, y : 0, width : 0, height : 0 }
    if(monitor != null) {
        gdk_monitor_get_geometry(monitor, &raw mut rect)
    }
    return Rect.make(rect.x, rect.y, rect.width, rect.height)
}

public func window_monitor_scale(index : int) : double {
    ensure_gtk_init()
    var display = gdk_display_get_default()
    var monitor = gdk_display_get_monitor(display, index)
    var scale = 1
    if(monitor != null) {
        scale = gdk_monitor_get_scale_factor(monitor)
    }
    return scale as double
}

public func window_set_user_data(w : *mut Window, data : *mut void) {
    w.user_data = data
    window_ctx_sync(w)
}

public func window_set_resize_callback(w : *mut Window, cb : ResizeCallback) {
    w.resize_cb = cb
    window_ctx_sync(w)
}

public func window_set_close_callback(w : *mut Window, cb : CloseCallback) {
    w.close_cb = cb
    window_ctx_sync(w)
}

public func window_set_focus_callback(w : *mut Window, cb : FocusCallback) {
    w.focus_cb = cb
    window_ctx_sync(w)
}

public func window_set_event_callback(w : *mut Window, cb : EventCallback) {
    w.event_cb = cb
    window_ctx_sync(w)
}

public func window_set_drop_callback(w : *mut Window, cb : DropCallback) {
    w.drop_cb = cb
    window_ctx_sync(w)
}

// On GTK, native menu/button activation is wired by the app through its own
// signal handlers, so command_cb is stored for API parity but not invoked.
public func window_set_command_callback(w : *mut Window, cb : CommandCallback) {
    w.command_cb = cb
    window_ctx_sync(w)
}

public func window_native_handle(w : *mut Window) : *mut void {
    return w.widget as *mut void
}

public func window_run() {
    g_in_main_loop = 1
    g_quit_by_destroy = 0
    gtk_main()
    g_in_main_loop = 0
}

// Returns 1 if the last window_run() returned because a window was destroyed
// (the user closed it) rather than because window_quit() was called. When 1,
// every widget owned by that window — and anything packed inside it — has
// already been finalized, so callers must not destroy them again.
//
// LIMITATION: the flag is global (the main loop is global), so for apps with
// several windows it reflects "a window was destroyed during the last run".
// After a close, call window_destroy() (guarded by this flag) before touching
// the window again — calling other window_* APIs on a closed window would
// dereference its dangling widget pointer.
// (Linux-only helper; not part of the cross-platform window API.)
public func window_quit_by_destroy() : int {
    return g_quit_by_destroy
}

public func window_quit() {
    if(g_in_main_loop != 0) {
        gtk_main_quit()
    }
}

func linux_empty_callback(data : *mut void) : int {
    // Return 0 so GTK removes the idle source after firing once.
    return 0
}

/// Post an empty event to wake up the message loop.
/// Useful for waking up the message loop from another thread.
public func window_post_empty_event() {
    g_idle_add(linux_empty_callback as *mut void, null)
}

// ===========================================================================
// clipboard (GTK)
// ===========================================================================

@extern public func gtk_clipboard_get(selection : *mut void) : *mut void
@extern public func gtk_clipboard_set_text(clipboard : *mut void, text : *char, len : int) : void
@extern public func gtk_clipboard_wait_for_text(clipboard : *mut void) : *char
@extern public func gtk_selection_data_get_text(data : *mut void) : *char

/// Lazily initialize GTK exactly once. The toolkit must be initialized before
/// any clipboard call; doing it here (rather than relying on the caller having
/// created a window first) keeps the clipboard API self-contained while only
/// paying the init cost a single time per process.
var g_gtk_initialized : bool = false
func ensure_gtk_init() {
    if(!g_gtk_initialized) {
        var argc : int = 0
        var argv : *mut *mut *char = null
        gtk_init_check(&raw mut argc, argv)
        g_gtk_initialized = true
    }
}

/// Get the current clipboard text content.
public func window_get_clipboard() : Option<string> {
    ensure_gtk_init()
    // GDK_SELECTION_CLIPBOARD = gdk_atom_intern("CLIPBOARD", 1)
    // We use a simpler approach: get the default clipboard
    var sel = gdk_atom_intern("CLIPBOARD", 1)
    var cb = gtk_clipboard_get(sel)
    if(cb == null) { return Option.None<string>() }
    var text = gtk_clipboard_wait_for_text(cb)
    if(text == null) { return Option.None<string>() }
    var result = string(text)
    g_free(text as *mut void)
    return Option.Some(result)
}

/// Set the clipboard text content.
public func window_set_clipboard(text : string_view) : bool {
    ensure_gtk_init()
    var sel = gdk_atom_intern("CLIPBOARD", 1)
    var cb = gtk_clipboard_get(sel)
    if(cb == null) { return false }
    gtk_clipboard_set_text(cb, text.data(), text.size() as int)
    return true
}

@extern public func gdk_atom_intern(atom_name : *char, only_if_exists : int) : *mut void

// ===========================================================================
// timer (GTK)
// ===========================================================================

@extern public func g_timeout_add(interval_ms : u32, function : *mut void, data : *mut void) : u32
@extern public func g_source_remove(tag : u32) : int

comptime const MAX_TIMERS : int = 16
var g_timer_cbs : [16]TimerCallback
var g_timer_data : [16]*mut void
var g_timer_used : [16]bool
var g_timer_ids : [16]u32

func linux_timer_wrapper(data : *mut void) : int {
    // data is the timer index (as a pointer)
    var idx = data as int
    if(idx >= 0 && idx < MAX_TIMERS && g_timer_used[idx] && g_timer_cbs[idx] != null) {
        g_timer_cbs[idx](g_timer_data[idx])
    }
    return 1 // return 1 to keep the timer running
}

/// Set a periodic timer.
public func window_set_timer(interval_ms : int, cb : TimerCallback, data : *mut void) : int {
    var idx = -1
    var i : int = 0
    while(i < MAX_TIMERS) {
        if(!g_timer_used[i]) {
            idx = i
            break
        }
        i += 1
    }
    if(idx < 0) { return 0 }
    g_timer_cbs[idx] = cb
    g_timer_data[idx] = data
    g_timer_used[idx] = true
    var tag = g_timeout_add(interval_ms as u32, linux_timer_wrapper as *mut void, idx as *mut void)
    g_timer_ids[idx] = tag
    return idx + 1
}

/// Cancel a previously set timer.
public func window_cancel_timer(timer_id : int) {
    if(timer_id <= 0 || timer_id > MAX_TIMERS) { return }
    var idx = timer_id - 1
    if(g_timer_used[idx]) {
        g_source_remove(g_timer_ids[idx])
        g_timer_used[idx] = false
        g_timer_cbs[idx] = null
        g_timer_data[idx] = null
    }
}

} // end namespace window
