// ---------------------------------------------------------------------------
// window library — cross-platform windowing.
//
// A single Window API shared by all backends. Backend files (win/win.ch,
// posix/linux.ch) implement the same public functions with the same
// semantics; this file holds the platform-independent types, constants and
// callback signatures.
//
// Coordinate conventions:
//   - window_set_size / window_size / window_set_position / window_position
//     work in LOGICAL units (96-dpi coordinates); the library scales them to
//     physical pixels using the window's DPI.
//   - The resize callback and window_client_size report PHYSICAL pixels (the
//     raw client area), which is what native code (child windows, GPU
//     surfaces) needs.
// ---------------------------------------------------------------------------

public namespace window {

using std::string;

public variant WindowError {
    PlatformNotSupported()
    CreateFailed(msg : string)
    InvalidState(msg : string)

    func message(&self) : string {
        switch(self) {
            PlatformNotSupported() => return string("window: platform not supported")
            CreateFailed(msg) => {
                var s = string("window: creation failed: ")
                s.append_string(&msg)
                return s
            }
            InvalidState(msg) => {
                var s = string("window: invalid state: ")
                s.append_string(&msg)
                return s
            }
        }
    }
}

// ===========================================================================
// geometry
// ===========================================================================

@direct_init
public struct Size {
    var width : int
    var height : int

    @make
    func make(width_ : int, height_ : int) : Size {
        return Size { width : width_, height : height_ }
    }
}

@direct_init
public struct Position {
    var x : int
    var y : int

    @make
    func make(x_ : int, y_ : int) : Position {
        return Position { x : x_, y : y_ }
    }
}

@direct_init
public struct Rect {
    var x : int
    var y : int
    var width : int
    var height : int

    @make
    func make(x_ : int, y_ : int, w_ : int, h_ : int) : Rect {
        return Rect { x : x_, y : y_, width : w_, height : h_ }
    }
}

// ===========================================================================
// input events
// ===========================================================================

// event kinds
public comptime const EVENT_KEY_DOWN : int = 0
public comptime const EVENT_KEY_UP : int = 1
public comptime const EVENT_CHAR : int = 2
public comptime const EVENT_MOUSE_DOWN : int = 3
public comptime const EVENT_MOUSE_UP : int = 4
public comptime const EVENT_MOUSE_MOVE : int = 5
public comptime const EVENT_MOUSE_WHEEL : int = 6

// mouse buttons
public comptime const MOUSE_LEFT : int = 0
public comptime const MOUSE_RIGHT : int = 1
public comptime const MOUSE_MIDDLE : int = 2

// modifier flags
public comptime const MOD_SHIFT : int = 1
public comptime const MOD_CTRL : int = 2
public comptime const MOD_ALT : int = 4

// cursor shapes
public comptime const CURSOR_ARROW : int = 0
public comptime const CURSOR_IBEAM : int = 1
public comptime const CURSOR_WAIT : int = 2
public comptime const CURSOR_CROSS : int = 3
public comptime const CURSOR_HAND : int = 4
public comptime const CURSOR_HELP : int = 5
public comptime const CURSOR_MOVE : int = 6
public comptime const CURSOR_RESIZE_NS : int = 7
public comptime const CURSOR_RESIZE_EW : int = 8
public comptime const CURSOR_RESIZE_NESW : int = 9
public comptime const CURSOR_RESIZE_NWSE : int = 10
public comptime const CURSOR_NO : int = 11
public comptime const CURSOR_APPSTARTING : int = 12
public comptime const CURSOR_SIZEALL : int = 13

@direct_init
public struct Event {
    var kind : int
    var x : int      // mouse: client x (physical); wheel: not used
    var y : int      // mouse: client y (physical); wheel: delta
    var button : int // mouse button for down/up
    var key : int    // key: platform key code (VK on Windows, GDK keyval on Linux)
    var key_char : uchar // printable character for EVENT_CHAR
    var modifiers : int  // MOD_* bit flags

    @make
    func make() : Event {
        return Event {
            kind : 0,
            x : 0,
            y : 0,
            button : 0,
            key : 0,
            key_char : 0,
            modifiers : 0
        }
    }
}

// ===========================================================================
// callbacks (all receive the user_data registered with window_set_user_data)
// ===========================================================================

public type ResizeCallback = (data : *mut void, width : int, height : int) => void
public type CloseCallback = (data : *mut void) => void
public type FocusCallback = (data : *mut void, focused : bool) => void
public type EventCallback = (data : *mut void, event : *mut Event) => void
public type DropCallback = (data : *mut void, path : *char) => void
public type CommandCallback = (data : *mut void, id : int) => void

} // end namespace window
