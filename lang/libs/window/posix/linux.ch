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
const GDK_KEY_PRESS_MASK = 1 << 0
const GDK_KEY_RELEASE_MASK = 1 << 1
const GDK_POINTER_MOTION_MASK = 1 << 6
const GDK_BUTTON_PRESS_MASK = 1 << 8
const GDK_BUTTON_RELEASE_MASK = 1 << 9
const GDK_SCROLL_MASK = 1 << 12

// geometry hints
const GDK_HINT_MIN_SIZE = 2 // 1 << 1
const GDK_HINT_MAX_SIZE = 4 // 1 << 2

// drag & drop
const GTK_DEST_DEFAULT_ALL = 7
const GDK_ACTION_COPY = 2

// ===========================================================================
// GTK / GDK functions
// ===========================================================================

@extern public func gtk_init(argc : *mut int, argv : *mut *mut *char) : int
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
    var cursor = gdk_cursor_new_from_name(gdk_display_get_default(), cursor_name_for_kind(w.cursor))
    if(cursor != null) {
        gdk_window_set_cursor(gdkwin, cursor)
    }
}

// ===========================================================================
// signal handlers
// ===========================================================================

func linux_on_destroy(widget : *mut GtkWidget, data : *mut void) {
    var w = data as *mut Window
    w.created = false
    w.widget = null
    gtk_main_quit()
}

func linux_on_size_allocate(widget : *mut GtkWidget, alloc : *mut GtkAllocation, data : *mut void) {
    var w = data as *mut Window
    if(w != null && w.resize_cb != null) {
        w.resize_cb(w.user_data, alloc.width, alloc.height)
    }
}

func linux_on_delete(widget : *mut GtkWidget, event : *mut void, data : *mut void) : int {
    var w = data as *mut Window
    if(w != null && w.close_cb != null) {
        w.close_cb(w.user_data)
    }
    return 0 // allow the window to close
}

func linux_on_focus_in(widget : *mut GtkWidget, event : *mut void, data : *mut void) : int {
    var w = data as *mut Window
    if(w != null && w.focus_cb != null) {
        w.focus_cb(w.user_data, true)
    }
    return 0
}

func linux_on_focus_out(widget : *mut GtkWidget, event : *mut void, data : *mut void) : int {
    var w = data as *mut Window
    if(w != null && w.focus_cb != null) {
        w.focus_cb(w.user_data, false)
    }
    return 0
}

func deliver_key_event(w : *mut Window, kind : int, event : *const GdkEventKey) {
    if(w.event_cb == null) {
        return
    }
    var ev = Event.make()
    ev.kind = kind
    ev.key = event.keyval as int
    ev.modifiers = 0
    if((event.state & 0x1) != 0) { ev.modifiers |= MOD_SHIFT } // GDK_SHIFT_MASK
    if((event.state & 0x4) != 0) { ev.modifiers |= MOD_CTRL }  // GDK_CONTROL_MASK
    if((event.state & 0x8) != 0) { ev.modifiers |= MOD_ALT }   // GDK_MOD1_MASK
    w.event_cb(w.user_data, &raw ev)
}

func linux_on_key_press(widget : *mut GtkWidget, event : *const GdkEventKey, data : *mut void) : int {
    var w = data as *mut Window
    if(w != null) {
        deliver_key_event(w, EVENT_KEY_DOWN, event)
    }
    return 0
}

func linux_on_key_release(widget : *mut GtkWidget, event : *const GdkEventKey, data : *mut void) : int {
    var w = data as *mut Window
    if(w != null) {
        deliver_key_event(w, EVENT_KEY_UP, event)
    }
    return 0
}

func deliver_mouse_event(w : *mut Window, kind : int, x : double, y : double, button : int, state : DWORD) {
    if(w.event_cb == null) {
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
    w.event_cb(w.user_data, &raw ev)
}

func linux_on_button_press(widget : *mut GtkWidget, event : *const GdkEventButton, data : *mut void) : int {
    var w = data as *mut Window
    if(w != null) {
        var button = MOUSE_LEFT
        if(event.button == 3) {
            button = MOUSE_RIGHT
        } else if(event.button == 2) {
            button = MOUSE_MIDDLE
        }
        deliver_mouse_event(w, EVENT_MOUSE_DOWN, event.x, event.y, button, event.state)
    }
    return 0
}

func linux_on_button_release(widget : *mut GtkWidget, event : *const GdkEventButton, data : *mut void) : int {
    var w = data as *mut Window
    if(w != null) {
        var button = MOUSE_LEFT
        if(event.button == 3) {
            button = MOUSE_RIGHT
        } else if(event.button == 2) {
            button = MOUSE_MIDDLE
        }
        deliver_mouse_event(w, EVENT_MOUSE_UP, event.x, event.y, button, event.state)
    }
    return 0
}

func linux_on_motion(widget : *mut GtkWidget, event : *const GdkEventMotion, data : *mut void) : int {
    var w = data as *mut Window
    if(w != null) {
        deliver_mouse_event(w, EVENT_MOUSE_MOVE, event.x, event.y, MOUSE_LEFT, event.state)
    }
    return 0
}

func linux_on_scroll(widget : *mut GtkWidget, event : *const GdkEventScroll, data : *mut void) : int {
    var w = data as *mut Window
    if(w != null && w.event_cb != null) {
        var ev = Event.make()
        ev.kind = EVENT_MOUSE_WHEEL
        ev.x = event.x as int
        ev.y = event.y as int
        ev.y = (event.delta_y * 100.0) as int // wheel delta (approx.)
        w.event_cb(w.user_data, &raw ev)
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
    var w = user_data as *mut Window
    if(w == null || w.drop_cb == null || data == null) {
        return
    }
    var uris = gtk_selection_data_get_uris(data)
    if(uris != null && *uris != null) {
        var err : *mut GError = null
        var path = g_filename_from_uri(*uris, null, &raw mut err)
        if(path != null) {
            w.drop_cb(w.user_data, path)
            g_free(path as *mut void)
        }
    }
}

// ===========================================================================
// public API
// ===========================================================================

public func window_create(w : *mut Window) : std::Result<std::Unit, WindowError> {
    if(w == null) {
        return std.Result.Err(WindowError.InvalidState(string("window_create: null window")))
    }
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

    // signals
    g_signal_connect_data(w.widget as *mut void, "destroy\0" as *char, linux_on_destroy as *mut void, w as *mut void, null, 0)
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
    if(w == null) {
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
    if(w == null) {
        return false
    }
    return w.created && w.widget != null
}

public func window_title(w : *mut Window) : string {
    return w.title
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
        w.visible = true
    }
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
    if(w.widget == null || path == null) {
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
    return gdk_display_get_n_monitors(gdk_display_get_default())
}

public func window_monitor_bounds(index : int) : Rect {
    var display = gdk_display_get_default()
    var monitor = gdk_display_get_monitor(display, index)
    var rect = GdkRectangle { x : 0, y : 0, width : 0, height : 0 }
    if(monitor != null) {
        gdk_monitor_get_geometry(monitor, &raw mut rect)
    }
    return Rect.make(rect.x, rect.y, rect.width, rect.height)
}

public func window_monitor_scale(index : int) : double {
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
}

public func window_set_resize_callback(w : *mut Window, cb : ResizeCallback) {
    w.resize_cb = cb
}

public func window_set_close_callback(w : *mut Window, cb : CloseCallback) {
    w.close_cb = cb
}

public func window_set_focus_callback(w : *mut Window, cb : FocusCallback) {
    w.focus_cb = cb
}

public func window_set_event_callback(w : *mut Window, cb : EventCallback) {
    w.event_cb = cb
}

public func window_set_drop_callback(w : *mut Window, cb : DropCallback) {
    w.drop_cb = cb
}

// On GTK, native menu/button activation is wired by the app through its own
// signal handlers, so command_cb is stored for API parity but not invoked.
public func window_set_command_callback(w : *mut Window, cb : CommandCallback) {
    w.command_cb = cb
}

public func window_native_handle(w : *mut Window) : *mut void {
    return w.widget as *mut void
}

public func window_run() {
    gtk_main()
}

public func window_quit() {
    gtk_main_quit()
}

} // end namespace window
