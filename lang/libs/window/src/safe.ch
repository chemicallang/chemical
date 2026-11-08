// safe — additive owning/borrowing façade over the window API.
//
// The raw surface (`window_create`, `window_set_*`, `window_run`, callback
// setters with `*mut void` user data) lives under `window::raw`. This file is
// the canonical normal-use surface:
//
//   * `window::safe::Window` is an owning handle created with `Window.new(&str)`
//     and destroyed by its `@delete` destructor or `close()`;
//   * every text argument is a `&str`, every query returns an owned/typed value;
//   * callbacks are `std::function` values stored by the handle, with explicit
//     `off_*` unregister methods — no `*mut void` user data crosses the API;
//   * the handle's only field is a private raw state pointer, so `Window` is
//     `!Send`/`!Sync` by construction (state stays on the creating thread).
//
// The state is heap-allocated exactly once, so the address registered as the
// native window's user data never moves when the safe handle itself is stored
// in a `Result`, a struct, or returned by value.

public namespace window {

public namespace safe {

using std::Option;
using std::Result;
using std::string;
using std::string_view;

// ---------------------------------------------------------------------------
// Callback signatures (owned captures; no user-data pointer)
// ---------------------------------------------------------------------------

public type ResizeHandler = std::function<(width : int, height : int) => void>
public type CloseHandler = std::function<() => void>
public type FocusHandler = std::function<(focused : bool) => void>
public type EventHandler = std::function<(event : &Event) => void>
public type DropHandler = std::function<(path : string_view) => void>
public type CommandHandler = std::function<(id : int) => void>
public type TimerHandler = std::function<() => void>

// One owned timer callback. The native timer receives the box address as user
// data; the box lives until `cancel_timer`/the window destructor frees it.
@direct_init
protected struct TimerBox {
    var cb : TimerHandler
}

// Empty typed handlers return a real `%capture` value (not a bare function
// pointer), so `off_*` releases the previous captures through normal assignment.
func empty_resize_handler() : ResizeHandler {
    return (width : int, height : int) => {}
}
func empty_close_handler() : CloseHandler {
    return () => {}
}
func empty_focus_handler() : FocusHandler {
    return (focused : bool) => {}
}
func empty_event_handler() : EventHandler {
    return (event : &Event) => {}
}
func empty_drop_handler() : DropHandler {
    return (path : string_view) => {}
}
func empty_command_handler() : CommandHandler {
    return (id : int) => {}
}

// ---------------------------------------------------------------------------
// Heap state (stable address for the lifetime of the native window)
// ---------------------------------------------------------------------------

@direct_init
protected struct WindowState {
    var inner : window::Window
    var closed : bool
    var has_resize : bool
    var resize_fn : ResizeHandler
    var has_close : bool
    var close_fn : CloseHandler
    var has_focus : bool
    var focus_fn : FocusHandler
    var has_event : bool
    var event_fn : EventHandler
    var has_drop : bool
    var drop_fn : DropHandler
    var has_command : bool
    var command_fn : CommandHandler
    // Owned timer callback boxes, indexed by (native timer id - 1). The array
    // is zeroed with the rest of the state, so unused slots are null.
    var timer_boxes : [16]*mut TimerBox
}

// ---------------------------------------------------------------------------
// Native trampolines: the native layer only ever sees the stable state pointer.
// ---------------------------------------------------------------------------

func on_resize_trampoline(data : *mut void, width : int, height : int) { unsafe {
    var st = data as *mut WindowState
    if(st == null || !st.has_resize) { return }
    st.resize_fn(width, height)
}}

func on_close_trampoline(data : *mut void) { unsafe {
    var st = data as *mut WindowState
    if(st == null || !st.has_close) { return }
    st.close_fn()
}}

func on_focus_trampoline(data : *mut void, focused : bool) { unsafe {
    var st = data as *mut WindowState
    if(st == null || !st.has_focus) { return }
    st.focus_fn(focused)
}}

func on_event_trampoline(data : *mut void, event : *mut Event) { unsafe {
    var st = data as *mut WindowState
    if(st == null || !st.has_event || event == null) { return }
    var ev = *event
    st.event_fn(&ev)
}}

func on_drop_trampoline(data : *mut void, path : *char) { unsafe {
    var st = data as *mut WindowState
    if(st == null || !st.has_drop || path == null) { return }
    st.drop_fn(unsafe(std::string_view.make_no_len(path)))
}}

func on_command_trampoline(data : *mut void, id : int) { unsafe {
    var st = data as *mut WindowState
    if(st == null || !st.has_command) { return }
    st.command_fn(id)
}}

func on_timer_trampoline(data : *mut void) { unsafe {
    var box = data as *mut TimerBox
    if(box == null) { return }
    box.cb()
}}

// ---------------------------------------------------------------------------
// Owning handle
// ---------------------------------------------------------------------------

public struct Window {

    // Private: safe code cannot forge, inspect, or move the native state out.
    private var state : *mut WindowState

    // Create the native window. Width/height are logical (96-dpi) units.
    public func new(title : &std::str, width : int = 800, height : int = 600) : Result<Window, WindowError> {
        unsafe {
            var st = new WindowState
            if(st == null) {
                return Result.Err(WindowError.CreateFailed(string("window: out of memory")))
            }
            memset(st as *mut void, 0, sizeof(WindowState))

            st.inner = window::Window.make()
            st.inner.title = string()
            st.inner.title.append_view(title.to_view())
            st.inner.width = width
            st.inner.height = height
            st.closed = false

            // Stable state pointer, not the (movable) safe handle.
            st.inner.user_data = st as *mut void
            st.inner.resize_cb = on_resize_trampoline
            st.inner.close_cb = on_close_trampoline
            st.inner.focus_cb = on_focus_trampoline
            st.inner.event_cb = on_event_trampoline
            st.inner.drop_cb = on_drop_trampoline
            st.inner.command_cb = on_command_trampoline

            var created = raw::window_create(&raw mut st.inner)
            if(created is Result.Err) {
                var Err(err) = created else unsafe { unreachable }
                delete st
                return Result.Err(err)
            }
            return Result.Ok(Window { state : st })
        }
    }

    public func is_open(&self) : bool {
        unsafe {
            var st = self.state
            if(st == null || st.closed) { return false }
            return raw::window_is_created(&raw mut st.inner)
        }
    }

    // ── title / geometry ─────────────────────────────────────────────────

    public func title(&self) : string {
        unsafe {
            var st = self.state
            if(st == null) { return string("") }
            return raw::window_title(&raw mut st.inner)
        }
    }

    public func set_title(&mut self, title : &std::str) {
        unsafe {
            var st = self.state
            if(st == null || st.closed) { return }
            var text = string()
            text.append_view(title.to_view())
            raw::window_set_title(&raw mut st.inner, text.data())
        }
    }

    public func size(&self) : Size {
        unsafe {
            var st = self.state
            if(st == null) { return Size.make(0, 0) }
            return raw::window_size(&raw mut st.inner)
        }
    }

    public func client_size(&self) : Size {
        unsafe {
            var st = self.state
            if(st == null) { return Size.make(0, 0) }
            return raw::window_client_size(&raw mut st.inner)
        }
    }

    public func set_size(&mut self, width : int, height : int) {
        unsafe {
            var st = self.state
            if(st == null || st.closed) { return }
            raw::window_set_size(&raw mut st.inner, width, height)
        }
    }

    public func position(&self) : Position {
        unsafe {
            var st = self.state
            if(st == null) { return Position.make(0, 0) }
            return raw::window_position(&raw mut st.inner)
        }
    }

    public func set_position(&mut self, x : int, y : int) {
        unsafe {
            var st = self.state
            if(st == null || st.closed) { return }
            raw::window_set_position(&raw mut st.inner, x, y)
        }
    }

    // ── visibility / lifetime ────────────────────────────────────────────

    public func show(&mut self) {
        unsafe {
            var st = self.state
            if(st == null || st.closed) { return }
            raw::window_show(&raw mut st.inner)
        }
    }

    public func hide(&mut self) {
        unsafe {
            var st = self.state
            if(st == null || st.closed) { return }
            raw::window_hide(&raw mut st.inner)
        }
    }

    public func focus(&mut self) {
        unsafe {
            var st = self.state
            if(st == null || st.closed) { return }
            raw::window_focus(&raw mut st.inner)
        }
    }

    // Close (destroy) the native window. Idempotent; the destructor is a no-op
    // afterwards.
    public func close(&mut self) {
        unsafe {
            var st = self.state
            if(st == null || st.closed) { return }
            raw::window_close(&raw mut st.inner)
            st.closed = true
        }
    }

    // ── appearance / behaviour ───────────────────────────────────────────

    public func set_cursor(&mut self, cursor : int) {
        unsafe {
            var st = self.state
            if(st == null || st.closed) { return }
            raw::window_set_cursor(&raw mut st.inner, cursor)
        }
    }

    public func set_icon(&mut self, path : &std::str) : Result<std::Unit, WindowError> {
        unsafe {
            var st = self.state
            if(st == null || st.closed) {
                return Result.Err(WindowError.InvalidState(string("window is closed")))
            }
            var p = string()
            p.append_view(path.to_view())
            raw::window_set_icon(&raw mut st.inner, p.data())
            return Result.Ok(std::Unit{})
        }
    }

    public func set_opacity(&mut self, opacity : double) {
        unsafe {
            var st = self.state
            if(st == null || st.closed) { return }
            raw::window_set_opacity(&raw mut st.inner, opacity)
        }
    }

    public func set_always_on_top(&mut self, on_top : bool) {
        unsafe {
            var st = self.state
            if(st == null || st.closed) { return }
            raw::window_set_always_on_top(&raw mut st.inner, on_top)
        }
    }

    public func set_decorated(&mut self, decorated : bool) {
        unsafe {
            var st = self.state
            if(st == null || st.closed) { return }
            raw::window_set_decorated(&raw mut st.inner, decorated)
        }
    }

    public func dpi(&self) : int {
        unsafe {
            var st = self.state
            if(st == null) { return 96 }
            return raw::window_dpi(&raw mut st.inner)
        }
    }

    public func scale_factor(&self) : double {
        unsafe {
            var st = self.state
            if(st == null) { return 1.0 }
            return raw::window_scale_factor(&raw mut st.inner)
        }
    }

    // ── monitors / clipboard ─────────────────────────────────────────────
    // These are process-global in the native backend; the instance methods are
    // ergonomic forwarding wrappers so a caller never needs `window::raw`.

    public func monitor_count(&self) : int {
        return raw::window_monitor_count()
    }

    public func monitor_bounds(&self, index : int) : Rect {
        return raw::window_monitor_bounds(index)
    }

    public func monitor_scale(&self, index : int) : double {
        return raw::window_monitor_scale(index)
    }

    // An owned copy of the clipboard text, or None when it is empty/unavailable.
    public func get_clipboard(&self) : Option<string> {
        return raw::window_get_clipboard()
    }

    public func set_clipboard(&self, text : &std::str) : bool {
        return raw::window_set_clipboard(text.to_view())
    }

    // Explicit raw platform handle access. The returned pointer is owned by the
    // native window and must not outlive it; all use is an unsafe boundary.
    public unsafe func native_handle(&self) : *mut void {
        unsafe {
            var st = self.state
            if(st == null) { return null }
            return raw::window_native_handle(&raw mut st.inner)
        }
    }

    // ── callbacks (owned captures) ───────────────────────────────────────

    public func on_resize(&mut self, handler : ResizeHandler) {
        unsafe {
            var st = self.state
            if(st == null) { return }
            st.resize_fn = handler
            st.has_resize = true
        }
    }

    // Unregistering replaces the stored closure with a no-op, releasing its
    // captures immediately instead of holding them until the window dies.
    public func off_resize(&mut self) {
        unsafe {
            var st = self.state
            if(st == null) { return }
            st.has_resize = false
            st.resize_fn = empty_resize_handler()
        }
    }

    public func on_close(&mut self, handler : CloseHandler) {
        unsafe {
            var st = self.state
            if(st == null) { return }
            st.close_fn = handler
            st.has_close = true
        }
    }

    public func off_close(&mut self) {
        unsafe {
            var st = self.state
            if(st == null) { return }
            st.has_close = false
            st.close_fn = empty_close_handler()
        }
    }

    public func on_focus(&mut self, handler : FocusHandler) {
        unsafe {
            var st = self.state
            if(st == null) { return }
            st.focus_fn = handler
            st.has_focus = true
        }
    }

    public func off_focus(&mut self) {
        unsafe {
            var st = self.state
            if(st == null) { return }
            st.has_focus = false
            st.focus_fn = empty_focus_handler()
        }
    }

    public func on_event(&mut self, handler : EventHandler) {
        unsafe {
            var st = self.state
            if(st == null) { return }
            st.event_fn = handler
            st.has_event = true
        }
    }

    public func off_event(&mut self) {
        unsafe {
            var st = self.state
            if(st == null) { return }
            st.has_event = false
            st.event_fn = empty_event_handler()
        }
    }

    // The path view is only valid during the callback (it points into the
    // native event); copy it if it must outlive the call.
    public func on_drop(&mut self, handler : DropHandler) {
        unsafe {
            var st = self.state
            if(st == null) { return }
            st.drop_fn = handler
            st.has_drop = true
        }
    }

    public func off_drop(&mut self) {
        unsafe {
            var st = self.state
            if(st == null) { return }
            st.has_drop = false
            st.drop_fn = empty_drop_handler()
        }
    }

    public func on_command(&mut self, handler : CommandHandler) {
        unsafe {
            var st = self.state
            if(st == null) { return }
            st.command_fn = handler
            st.has_command = true
        }
    }

    public func off_command(&mut self) {
        unsafe {
            var st = self.state
            if(st == null) { return }
            st.has_command = false
            st.command_fn = empty_command_handler()
        }
    }

    // ── timers (owned callback) ──────────────────────────────────────────

    // Register a periodic timer. The returned id is passed to `cancel_timer`.
    // The callback is owned by the handle and released by `cancel_timer` or
    // when the window is destroyed.
    public func set_timer(&mut self, interval_ms : int, handler : TimerHandler) : Result<int, WindowError> {
        unsafe {
            var st = self.state
            if(st == null || st.closed) {
                return Result.Err(WindowError.InvalidState(string("window is closed")))
            }
            var box = new TimerBox
            if(box == null) {
                return Result.Err(WindowError.CreateFailed(string("window: out of memory")))
            }
            memset(box as *mut void, 0, sizeof(TimerBox))
            box.cb = handler
            const id = raw::window_set_timer(interval_ms, on_timer_trampoline, box as *mut void)
            if(id <= 0 || id > 16) {
                delete box
                return Result.Err(WindowError.CreateFailed(string("window: no timer slot available")))
            }
            st.timer_boxes[id - 1] = box
            return Result.Ok(id)
        }
    }

    public func cancel_timer(&mut self, id : int) {
        unsafe {
            var st = self.state
            if(st == null) { return }
            raw::window_cancel_timer(id)
            if(id > 0 && id <= 16) {
                var box = st.timer_boxes[id - 1]
                if(box != null) {
                    delete box
                    st.timer_boxes[id - 1] = null
                }
            }
        }
    }

    @delete
    func destruct(&self) {
        unsafe {
            var st = self.state
            if(st == null) { return }
            // Stop and free every owned timer before tearing down native state.
            for(var i : int = 0; i < 16; i++) {
                if(st.timer_boxes[i] != null) {
                    raw::window_cancel_timer(i + 1)
                    delete st.timer_boxes[i]
                    st.timer_boxes[i] = null
                }
            }
            if(!st.closed && st.inner.created) {
                raw::window_destroy(&raw mut st.inner)
            }
            delete st
        }
    }
}

// A window handle owns native UI thread state; it must never cross threads.
impl !Send for Window { }
impl !Sync for Window { }

} // namespace safe

} // namespace window
