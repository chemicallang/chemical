// ---------------------------------------------------------------------------
// win.ch — Windows backend for the window library (pure Win32).
//
// Ported from the windowing code that previously lived in the webview
// library (lang/libs/webview/win/win.ch), which itself was a faithful port of
// lang/compiled/webview-c/wvwin.c. The top-level window, its window proc,
// DPI handling, min/max constraints, icon and show/hide logic were moved here
// and extended with: position, fullscreen, maximize/minimize/restore, cursor,
// per-window icon, opacity, always-on-top, decorations, monitor info,
// keyboard/mouse events and drag & drop.
//
// Sizes passed to window_set_size / window_create are LOGICAL (96-dpi);
// they are scaled to physical pixels via the window's DPI (the webview
// backend already did this for webview_set_size).
// ---------------------------------------------------------------------------

public namespace window {

using std::string;

// ===========================================================================
// Win32 types (the ones cstd does not provide)
// ===========================================================================

public type HWND = HANDLE
public type HICON = HANDLE
public type HMENU = HANDLE
public type HBRUSH = HANDLE
public type HCURSOR = HANDLE
public type HMONITOR = HANDLE
public type HDC = *mut void
public type SHORT = i16
/** window class atom (RegisterClassExW). */
public type ATOM = WORD

public type WNDPROC = (hwnd : HWND, msg : UINT, wp : WPARAM, lp : LPARAM) => LRESULT
public type LPCREATESTRUCTW = *mut CREATESTRUCTW
public type LPMINMAXINFO = *mut MINMAXINFO
public type LPRECT = *mut RECT
public type MONITORENUMPROC = (hmon : HMONITOR, hdc : HDC, lprc : *mut RECT, data : LPARAM) => BOOL

// ===========================================================================
// Win32 structs
// ===========================================================================

public struct POINT {
    var x : LONG
    var y : LONG
}

public struct RECT {
    var left : LONG
    var top : LONG
    var right : LONG
    var bottom : LONG
}

public struct MSG {
    var hwnd : HWND
    var message : UINT
    var wParam : WPARAM
    var lParam : LPARAM
    var time : DWORD
    var pt : POINT
}

public struct MINMAXINFO {
    var ptReserved : POINT
    var ptMaxSize : POINT
    var ptMaxPosition : POINT
    var ptMinTrackSize : POINT
    var ptMaxTrackSize : POINT
}

public struct CREATESTRUCTW {
    var lpCreateParams : LPVOID
    var hInstance : HINSTANCE
    var hMenu : HMENU
    var hwndParent : HWND
    var cy : int
    var cx : int
    var y : int
    var x : int
    var style : LONG
    var lpszName : LPCWSTR
    var lpszClass : LPCWSTR
    var dwExStyle : DWORD
}

public struct WNDCLASSEXW {
    var cbSize : UINT
    var style : UINT
    var lpfnWndProc : WNDPROC
    var cbClsExtra : int
    var cbWndExtra : int
    var hInstance : HINSTANCE
    var hIcon : HICON
    var hCursor : HCURSOR
    var hbrBackground : HBRUSH
    var lpszMenuName : LPCWSTR
    var lpszClassName : LPCWSTR
    var hIconSm : HICON
}

public struct MONITORINFO {
    var cbSize : DWORD
    var rcMonitor : RECT
    var rcWork : RECT
    var dwFlags : DWORD
}

// ===========================================================================
// Win32 constants
// ===========================================================================

// DPI
public comptime const DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 : *mut void = -4 as *mut void

// window messages
public comptime const WM_NCCREATE : UINT = 0x0081 as UINT
public comptime const WM_SIZE : UINT = 0x0005 as UINT
public comptime const WM_COMMAND : UINT = 0x0111 as UINT
public comptime const WM_DESTROY : UINT = 0x0002 as UINT
public comptime const WM_CLOSE : UINT = 0x0010 as UINT
public comptime const WM_GETMINMAXINFO : UINT = 0x0024 as UINT
public comptime const WM_ACTIVATE : UINT = 0x0006 as UINT
public comptime const WM_SETCURSOR : UINT = 0x0020 as UINT
public comptime const WM_SETICON : UINT = 0x0080 as UINT
public comptime const WM_DROPFILES : UINT = 0x0233 as UINT
public comptime const WM_KEYDOWN : UINT = 0x0100 as UINT
public comptime const WM_KEYUP : UINT = 0x0101 as UINT
public comptime const WM_CHAR : UINT = 0x0102 as UINT
public comptime const WM_MOUSEMOVE : UINT = 0x0200 as UINT
public comptime const WM_LBUTTONDOWN : UINT = 0x0201 as UINT
public comptime const WM_LBUTTONUP : UINT = 0x0202 as UINT
public comptime const WM_RBUTTONDOWN : UINT = 0x0204 as UINT
public comptime const WM_RBUTTONUP : UINT = 0x0205 as UINT
public comptime const WM_MBUTTONDOWN : UINT = 0x0207 as UINT
public comptime const WM_MBUTTONUP : UINT = 0x0208 as UINT
public comptime const WM_MOUSEWHEEL : UINT = 0x020A as UINT

// window styles
public comptime const WS_OVERLAPPEDWINDOW : DWORD = 0x00CF0000 as DWORD
public comptime const WS_POPUP : DWORD = 0x80000000 as DWORD
public comptime const WS_CAPTION : DWORD = 0x00C00000 as DWORD
public comptime const WS_THICKFRAME : DWORD = 0x00040000 as DWORD
public comptime const WS_MAXIMIZEBOX : DWORD = 0x00010000 as DWORD
public comptime const WS_MINIMIZEBOX : DWORD = 0x00020000 as DWORD
public comptime const WS_EX_LAYERED : DWORD = 0x00080000 as DWORD

// GetWindowLongPtrW / SetWindowLongPtrW indexes
public comptime const GWL_STYLE : int = -16
public comptime const GWL_EXSTYLE : int = -20
public comptime const GWLP_USERDATA : int = -21

// ShowWindow / SetWindowPos
public comptime const SW_SHOW : int = 5
public comptime const SW_HIDE : int = 0
public comptime const SW_MAXIMIZE : int = 3
public comptime const SW_MINIMIZE : int = 6
public comptime const SW_RESTORE : int = 9
public comptime const SWP_NOSIZE : UINT = 0x0001 as UINT
public comptime const SWP_NOMOVE : UINT = 0x0002 as UINT
public comptime const SWP_NOZORDER : UINT = 0x0004 as UINT
public comptime const SWP_NOACTIVATE : UINT = 0x0010 as UINT
public comptime const SWP_FRAMECHANGED : UINT = 0x0020 as UINT
public comptime const HWND_TOPMOST : HWND = -1 as HWND
public comptime const HWND_NOTOPMOST : HWND = -2 as HWND

// CreateWindow position
public comptime const CW_USEDEFAULT : int = 0x80000000 as int

// LoadImage
public comptime const IDI_APPLICATION : LPCWSTR = 32512 as LPCWSTR
public comptime const IMAGE_ICON : UINT = 1 as UINT
public comptime const LR_DEFAULTCOLOR : UINT = 0x0000 as UINT
public comptime const LR_LOADFROMFILE : UINT = 0x0010 as UINT
public comptime const LR_DEFAULTSIZE : UINT = 0x0040 as UINT
public comptime const SM_CXICON : int = 11
public comptime const SM_CYICON : int = 12
public comptime const SM_CXSCREEN : int = 0
public comptime const SM_CYSCREEN : int = 1
public comptime const SM_CMONITORS : int = 80
public comptime const ICON_SMALL : WPARAM = 0 as WPARAM
public comptime const ICON_BIG : WPARAM = 1 as WPARAM

// layered window attributes
public comptime const LWA_ALPHA : DWORD = 0x2 as DWORD

// monitor
public comptime const MONITOR_DEFAULTTONEAREST : DWORD = 2 as DWORD

// virtual key codes
public comptime const VK_SHIFT : int = 0x10
public comptime const VK_CONTROL : int = 0x11
public comptime const VK_MENU : int = 0x12

// ===========================================================================
// Win32 functions not provided by cstd
// ===========================================================================

// kernel32
@extern @stdcall @dllimport public func GetModuleHandleW(lpModuleName : LPCWSTR) : HMODULE
@extern @stdcall @dllimport public func LoadLibraryW(lpLibFileName : LPCWSTR) : HMODULE

// user32 (DPI)
@extern @stdcall @dllimport public func SetProcessDpiAwarenessContext(value : *mut void) : BOOL
@extern @stdcall @dllimport public func EnableNonClientDpiScaling(hwnd : HWND) : BOOL
@extern @stdcall @dllimport public func GetDpiForWindow(hwnd : HWND) : UINT
@extern @stdcall @dllimport public func AdjustWindowRectExForDpi(lpRect : LPRECT, dwStyle : DWORD, bMenu : BOOL, dwExStyle : DWORD, dpi : UINT) : BOOL

// NOTE: GetDpiForSystem and SetLayeredWindowAttributes are missing from tcc's
// bundled user32.def, so they are resolved at runtime via GetProcAddress (the
// same pattern the webview loader uses for WebView2Loader.dll).
public type GetDpiForSystemFn = () => UINT
public type SetLayeredWindowAttributesFn = (hwnd : HWND, crKey : DWORD, bAlpha : BYTE, dwFlags : DWORD) => BOOL

var g_user32 : HMODULE = null
var g_dpi_system_fn : GetDpiForSystemFn = null
var g_layered_fn : SetLayeredWindowAttributesFn = null

func win_user32() : HMODULE {
    if(g_user32 == null) {
        var wname : [16]ushort
        widen_to_buf("user32.dll", &raw mut wname[0], 16)
        g_user32 = LoadLibraryW(&raw wname[0] as LPCWSTR)
    }
    return g_user32
}

func win_get_dpi_for_system() : UINT {
    if(g_dpi_system_fn == null) {
        var m = win_user32()
        if(m != null) {
            g_dpi_system_fn = GetProcAddress(m, "GetDpiForSystem") as GetDpiForSystemFn
        }
    }
    if(g_dpi_system_fn == null) {
        return 96
    }
    return g_dpi_system_fn()
}

func win_set_layered_alpha(hwnd : HWND, alpha : BYTE) {
    if(g_layered_fn == null) {
        var m = win_user32()
        if(m != null) {
            g_layered_fn = GetProcAddress(m, "SetLayeredWindowAttributes") as SetLayeredWindowAttributesFn
        }
    }
    if(g_layered_fn == null) {
        return
    }
    g_layered_fn(hwnd, 0, alpha, LWA_ALPHA)
}

// user32 (windows)
@extern @stdcall @dllimport public func RegisterClassExW(lpWndClass : *mut WNDCLASSEXW) : ATOM
@extern @stdcall @dllimport public func CreateWindowExW(
    dwExStyle : DWORD,
    lpClassName : LPCWSTR,
    lpWindowName : LPCWSTR,
    dwStyle : DWORD,
    x : int,
    y : int,
    nWidth : int,
    nHeight : int,
    hWndParent : HWND,
    hMenu : HMENU,
    hInstance : HINSTANCE,
    lpParam : LPVOID
) : HWND
@extern @stdcall @dllimport public func DefWindowProcW(hwnd : HWND, msg : UINT, wp : WPARAM, lp : LPARAM) : LRESULT
@extern @stdcall @dllimport public func GetWindowLongPtrW(hwnd : HWND, nIndex : int) : LONG_PTR
@extern @stdcall @dllimport public func SetWindowLongPtrW(hwnd : HWND, nIndex : int, dwNewLong : LONG_PTR) : LONG_PTR
@extern @stdcall @dllimport public func SetWindowTextW(hwnd : HWND, lpString : LPCWSTR) : BOOL
@extern @stdcall @dllimport public func GetClientRect(hwnd : HWND, lpRect : LPRECT) : BOOL
@extern @stdcall @dllimport public func GetWindowRect(hwnd : HWND, lpRect : LPRECT) : BOOL
@extern @stdcall @dllimport public func MoveWindow(hwnd : HWND, x : int, y : int, nWidth : int, nHeight : int, bRepaint : BOOL) : BOOL
@extern @stdcall @dllimport public func DestroyWindow(hwnd : HWND) : BOOL
@extern @stdcall @dllimport public func PostQuitMessage(nExitCode : int) : void
@extern @stdcall @dllimport public func GetMessageW(lpMsg : *mut MSG, hWnd : HWND, wMsgFilterMin : UINT, wMsgFilterMax : UINT) : BOOL
@extern @stdcall @dllimport public func TranslateMessage(lpMsg : *mut MSG) : BOOL
@extern @stdcall @dllimport public func DispatchMessageW(lpMsg : *mut MSG) : LRESULT
@extern @stdcall @dllimport public func ShowWindow(hwnd : HWND, nCmdShow : int) : BOOL
@extern @stdcall @dllimport public func UpdateWindow(hwnd : HWND) : BOOL
@extern @stdcall @dllimport public func SetFocus(hwnd : HWND) : HWND
@extern @stdcall @dllimport public func SetForegroundWindow(hwnd : HWND) : BOOL
@extern @stdcall @dllimport public func SetWindowPos(hwnd : HWND, hWndInsertAfter : HWND, x : int, y : int, cx : int, cy : int, uFlags : UINT) : BOOL
@extern @stdcall @dllimport public func SendMessageW(hwnd : HWND, msg : UINT, wp : WPARAM, lp : LPARAM) : LRESULT
@extern @stdcall @dllimport public func GetKeyState(nVirtKey : int) : SHORT
@extern @stdcall @dllimport public func LoadCursorW(hInstance : HINSTANCE, name : LPCWSTR) : HCURSOR
@extern @stdcall @dllimport public func SetCursor(hCursor : HCURSOR) : HCURSOR
@extern @stdcall @dllimport public func MonitorFromWindow(hwnd : HWND, dwFlags : DWORD) : HMONITOR
@extern @stdcall @dllimport public func GetMonitorInfoW(hmonitor : HMONITOR, lpmi : *mut MONITORINFO) : BOOL
@extern @stdcall @dllimport public func EnumDisplayMonitors(hdc : HDC, lprcClip : LPRECT, proc : MONITORENUMPROC, data : LPARAM) : BOOL
@extern @stdcall @dllimport public func GetSystemMetrics(nIndex : int) : int
// NOTE: the SDK header maps LoadImage -> LoadImageW (the real user32 export).
@extern @stdcall @dllimport public func LoadImageW(hInstance : HINSTANCE, name : LPCWSTR, type : UINT, cx : int, cy : int, fuLoad : UINT) : HANDLE

// shell32 (drag & drop)
@extern @stdcall @dllimport public func DragAcceptFiles(hwnd : HWND, fAccept : BOOL) : void
@extern @stdcall @dllimport public func DragQueryFileW(hdrop : HANDLE, iFile : UINT, lpszFile : LPWSTR, cch : UINT) : UINT
@extern @stdcall @dllimport public func DragFinish(hdrop : HANDLE) : void

// ===========================================================================
// the Window struct
// ===========================================================================

@direct_init
public struct Window {
    // native handle (null until created)
    var hwnd : HWND

    // logical geometry (window_set_size/window_size work in logical units)
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
            hwnd : null,
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

func widen_to_buf(utf8 : *char, buf : *mut ushort, cap : size_t) {
    if(buf == null || cap == 0) {
        return
    }
    var n = MultiByteToWideChar(65001, 0, utf8, -1, buf, cap as int) // CP_UTF8
    if(n <= 0) {
        buf[0] = 0
    }
}

func wide_to_utf8(wide : *ushort, buf : *mut char, cap : size_t) {
    if(wide == null || buf == null || cap == 0) {
        return
    }
    var n = WideCharToMultiByte(65001, 0, wide as *u16, -1, buf, cap as int, null, null) // CP_UTF8
    if(n <= 0) {
        buf[0] = 0
    }
}

func current_modifiers() : int {
    var m = 0
    if((GetKeyState(VK_SHIFT) & 0x8000) != 0) {
        m |= MOD_SHIFT
    }
    if((GetKeyState(VK_CONTROL) & 0x8000) != 0) {
        m |= MOD_CTRL
    }
    if((GetKeyState(VK_MENU) & 0x8000) != 0) {
        m |= MOD_ALT
    }
    return m
}

func cursor_for_kind(kind : int) : HCURSOR {
    switch(kind) {
        CURSOR_IBEAM => return LoadCursorW(null, 32513 as LPCWSTR)
        CURSOR_WAIT => return LoadCursorW(null, 32514 as LPCWSTR)
        CURSOR_CROSS => return LoadCursorW(null, 32515 as LPCWSTR)
        CURSOR_HAND => return LoadCursorW(null, 32649 as LPCWSTR)
        CURSOR_HELP => return LoadCursorW(null, 32651 as LPCWSTR)
        CURSOR_MOVE => return LoadCursorW(null, 32646 as LPCWSTR)
        CURSOR_RESIZE_NS => return LoadCursorW(null, 32645 as LPCWSTR)
        CURSOR_RESIZE_EW => return LoadCursorW(null, 32644 as LPCWSTR)
        CURSOR_RESIZE_NESW => return LoadCursorW(null, 32643 as LPCWSTR)
        CURSOR_RESIZE_NWSE => return LoadCursorW(null, 32642 as LPCWSTR)
        CURSOR_NO => return LoadCursorW(null, 32648 as LPCWSTR)
        CURSOR_APPSTARTING => return LoadCursorW(null, 32650 as LPCWSTR)
        CURSOR_SIZEALL => return LoadCursorW(null, 32646 as LPCWSTR)
        default => return LoadCursorW(null, 32512 as LPCWSTR) // arrow
    }
}

// ===========================================================================
// window procedure
// ===========================================================================

func deliver_key(w : *mut Window, kind : int, key : int) {
    if(w.event_cb == null) {
        return
    }
    var ev = Event.make()
    ev.kind = kind
    ev.key = key
    ev.modifiers = current_modifiers()
    w.event_cb(w.user_data, &raw mut ev)
}

func deliver_char(w : *mut Window, ch : int) {
    if(w.event_cb == null) {
        return
    }
    var ev = Event.make()
    ev.kind = EVENT_CHAR
    ev.key_char = (ch & 0xFF) as uchar
    ev.modifiers = current_modifiers()
    w.event_cb(w.user_data, &raw mut ev)
}

func deliver_mouse(w : *mut Window, kind : int, button : int, lp : LPARAM) {
    if(w.event_cb == null) {
        return
    }
    var ev = Event.make()
    ev.kind = kind
    ev.button = button
    ev.x = ((lp as DWORD) & (0xFFFF as DWORD)) as int
    ev.y = (((lp as DWORD) >> 16) & (0xFFFF as DWORD)) as int
    ev.modifiers = current_modifiers()
    w.event_cb(w.user_data, &raw mut ev)
}

func win_window_proc(hwnd : HWND, msg : UINT, wp : WPARAM, lp : LPARAM) : LRESULT {
    var w = GetWindowLongPtrW(hwnd, GWLP_USERDATA) as *mut Window
    if(msg == WM_NCCREATE) {
        var cs = lp as *mut CREATESTRUCTW
        w = cs.lpCreateParams as *mut Window
        w.hwnd = hwnd
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, w as LONG_PTR)
        EnableNonClientDpiScaling(hwnd)
        DragAcceptFiles(hwnd, 1)
        return 1
    }
    if(w == null) {
        return DefWindowProcW(hwnd, msg, wp, lp)
    }
    switch(msg) {
        WM_SIZE => {
            var cw = ((lp as DWORD) & (0xFFFF as DWORD)) as int
            var ch = (((lp as DWORD) >> 16) & (0xFFFF as DWORD)) as int
            if(w.resize_cb != null) {
                w.resize_cb(w.user_data, cw, ch)
            }
        }
        WM_CLOSE => {
            if(w.close_cb != null) {
                w.close_cb(w.user_data)
            }
            DestroyWindow(hwnd)
        }
        WM_DESTROY => {
            w.created = false
            w.hwnd = null
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0)
            PostQuitMessage(0)
        }
        WM_GETMINMAXINFO => {
            var lpmmi = lp as *mut MINMAXINFO
            if(w.max_w > 0 && w.max_h > 0) {
                lpmmi.ptMaxSize = POINT { x : w.max_w, y : w.max_h }
                lpmmi.ptMaxTrackSize = POINT { x : w.max_w, y : w.max_h }
            }
            if(w.min_w > 0 && w.min_h > 0) {
                lpmmi.ptMinTrackSize = POINT { x : w.min_w, y : w.min_h }
            }
        }
        WM_ACTIVATE => {
            var wplow = (wp as DWORD) & (0xFFFF as DWORD)
            if(w.focus_cb != null) {
                w.focus_cb(w.user_data, wplow != 0)
            }
        }
        WM_COMMAND => {
            // Deliver only click/selection commands (notification code 0,
            // e.g. BN_CLICKED for buttons / menu selection). Buttons also send
            // BN_SETFOCUS/BN_KILLFOCUS notifications on mere focus changes —
            // filtering on the hiword prevents those from firing the callback.
            if(w.command_cb != null && (((wp as DWORD) >> 16) & (0xFFFF as DWORD)) == 0) {
                var id = (wp as DWORD) & (0xFFFF as DWORD)
                w.command_cb(w.user_data, id as int)
            }
        }
        WM_SETCURSOR => {
            var cursor = cursor_for_kind(w.cursor)
            SetCursor(cursor)
            return 1
        }
        WM_KEYDOWN => {
            deliver_key(w, EVENT_KEY_DOWN, wp as int)
        }
        WM_KEYUP => {
            deliver_key(w, EVENT_KEY_UP, wp as int)
        }
        WM_CHAR => {
            deliver_char(w, wp as int)
        }
        WM_MOUSEMOVE => {
            deliver_mouse(w, EVENT_MOUSE_MOVE, MOUSE_LEFT, lp)
        }
        WM_LBUTTONDOWN => {
            deliver_mouse(w, EVENT_MOUSE_DOWN, MOUSE_LEFT, lp)
        }
        WM_LBUTTONUP => {
            deliver_mouse(w, EVENT_MOUSE_UP, MOUSE_LEFT, lp)
        }
        WM_RBUTTONDOWN => {
            deliver_mouse(w, EVENT_MOUSE_DOWN, MOUSE_RIGHT, lp)
        }
        WM_RBUTTONUP => {
            deliver_mouse(w, EVENT_MOUSE_UP, MOUSE_RIGHT, lp)
        }
        WM_MBUTTONDOWN => {
            deliver_mouse(w, EVENT_MOUSE_DOWN, MOUSE_MIDDLE, lp)
        }
        WM_MBUTTONUP => {
            deliver_mouse(w, EVENT_MOUSE_UP, MOUSE_MIDDLE, lp)
        }
        WM_MOUSEWHEEL => {
            if(w.event_cb != null) {
                var ev = Event.make()
                ev.kind = EVENT_MOUSE_WHEEL
                ev.y = ((wp as DWORD) >> 16) as int // wheel delta
                ev.modifiers = current_modifiers()
                w.event_cb(w.user_data, &raw mut ev)
            }
        }
        WM_DROPFILES => {
            if(w.drop_cb != null) {
                var hdrop = wp as HANDLE
                var count = DragQueryFileW(hdrop, 0xFFFFFFFF as UINT, null, 0)
                if(count > 0) {
                    var buf : [1024]ushort
                    var n = DragQueryFileW(hdrop, 0, &raw mut buf[0], 1024)
                    if(n > 0) {
                        var utf8_buf : [2048]char
                        wide_to_utf8(&raw buf[0], &raw mut utf8_buf[0], 2048)
                        w.drop_cb(w.user_data, &raw utf8_buf[0])
                    }
                }
                DragFinish(hdrop)
            }
        }
        default => {
            return DefWindowProcW(hwnd, msg, wp, lp)
        }
    }
    return 0
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

    var hInstance = GetModuleHandleW(null)

    // Be DPI aware (best-effort; the call fails harmlessly if already set).
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)

    // register the window class
    var wc = zeroed<WNDCLASSEXW>()
    wc.cbSize = sizeof(WNDCLASSEXW) as UINT
    wc.hInstance = hInstance
    var wc_class_name : [64]ushort
    widen_to_buf("chem_window", &raw mut wc_class_name[0], 64)
    wc.lpszClassName = &raw wc_class_name[0]
    wc.lpfnWndProc = win_window_proc as WNDPROC
    wc.hIcon = LoadImageW(hInstance, IDI_APPLICATION, IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR) as HICON
    RegisterClassExW(&raw mut wc)

    // scale the requested logical size to physical pixels for creation
    w.dpi = win_get_dpi_for_system() as int
    if(w.dpi <= 0) {
        w.dpi = 96
    }
    var phys_w = (w.width * w.dpi) / 96
    var phys_h = (w.height * w.dpi) / 96

    var title_buf : [512]ushort
    widen_to_buf(w.title.data(), &raw mut title_buf[0], 512)

    w.hwnd = CreateWindowExW(
        0,
        &raw wc_class_name[0],
        &raw title_buf[0],
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        phys_w,
        phys_h,
        null,
        null,
        hInstance,
        w as *mut void
    )
    if(w.hwnd == null) {
        return std.Result.Err(WindowError.CreateFailed(string("CreateWindowExW failed")))
    }
    w.created = true

    // Re-check the DPI of the monitor the window actually opened on and
    // rescale if the requested size was computed for a different DPI.
    var actual_dpi = GetDpiForWindow(w.hwnd) as int
    if(actual_dpi > 0 && actual_dpi != w.dpi) {
        w.dpi = actual_dpi
        SetWindowPos(
            w.hwnd,
            null,
            0,
            0,
            (w.width * w.dpi) / 96,
            (w.height * w.dpi) / 96,
            SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE
        )
    }

    return std.Result.Ok(std::Unit{})
}

public func window_destroy(w : *mut Window) {
    if(w == null) {
        return
    }
    if(w.hwnd != null) {
        DestroyWindow(w.hwnd)
        w.hwnd = null
    }
    w.created = false
    w.visible = false
}

public func window_is_created(w : *mut Window) : bool {
    if(w == null) {
        return false
    }
    return w.created && w.hwnd != null
}

// --- title ---

public func window_title(w : *mut Window) : string {
    return w.title
}

public func window_set_title(w : *mut Window, title : *char) {
    w.title = string("")
    w.title.append_char_ptr(title)
    if(w.hwnd != null) {
        var wbuf : [512]ushort
        widen_to_buf(title, &raw mut wbuf[0], 512)
        SetWindowTextW(w.hwnd, &raw wbuf[0])
    }
}

// --- size / position (logical units) ---

public func window_size(w : *mut Window) : Size {
    return Size.make(w.width, w.height)
}

// Physical client size (for native layout/rendering code).
public func window_client_size(w : *mut Window) : Size {
    if(w.hwnd != null) {
        var rc : RECT
        if(GetClientRect(w.hwnd, &raw mut rc)) {
            return Size.make(rc.right - rc.left, rc.bottom - rc.top)
        }
    }
    return Size.make(w.width, w.height)
}

public func window_set_size(w : *mut Window, width : int, height : int) {
    w.width = width
    w.height = height
    if(w.hwnd == null) {
        return
    }
    var dpi = GetDpiForWindow(w.hwnd) as int
    if(dpi <= 0) {
        dpi = 96
    }
    w.dpi = dpi
    var scaled_w = (width * dpi) / 96
    var scaled_h = (height * dpi) / 96
    var style = GetWindowLongPtrW(w.hwnd, GWL_STYLE)
    var r = RECT { left : 0, top : 0, right : scaled_w, bottom : scaled_h }
    AdjustWindowRectExForDpi(&raw mut r, style as DWORD, 0, 0, dpi as UINT)
    SetWindowPos(
        w.hwnd,
        null,
        0,
        0,
        r.right - r.left,
        r.bottom - r.top,
        SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE | SWP_FRAMECHANGED
    )
}

public func window_position(w : *mut Window) : Position {
    if(w.hwnd != null) {
        var rc : RECT
        if(GetWindowRect(w.hwnd, &raw mut rc)) {
            return Position.make(rc.left, rc.top)
        }
    }
    return Position.make(w.x, w.y)
}

public func window_set_position(w : *mut Window, x : int, y : int) {
    w.x = x
    w.y = y
    if(w.hwnd != null) {
        SetWindowPos(w.hwnd, null, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE)
    }
}

// --- min / max size (logical units) ---

public func window_min_size(w : *mut Window) : Size {
    return Size.make(w.min_w, w.min_h)
}

public func window_max_size(w : *mut Window) : Size {
    return Size.make(w.max_w, w.max_h)
}

public func window_set_min_size(w : *mut Window, width : int, height : int) {
    w.min_w = width
    w.min_h = height
    // the constraint is applied by WM_GETMINMAXINFO on the next resize
}

public func window_set_max_size(w : *mut Window, width : int, height : int) {
    w.max_w = width
    w.max_h = height
    // the constraint is applied by WM_GETMINMAXINFO on the next resize
}

// --- state ---

public func window_set_fullscreen(w : *mut Window, fullscreen : bool) {
    if(w.hwnd == null || fullscreen == w.fullscreen) {
        return
    }
    if(fullscreen) {
        w.saved_style = GetWindowLongPtrW(w.hwnd, GWL_STYLE)
        GetWindowRect(w.hwnd, &raw mut w.saved_rect)
        var mi : MONITORINFO
        mi.cbSize = sizeof(MONITORINFO) as DWORD
        GetMonitorInfoW(MonitorFromWindow(w.hwnd, MONITOR_DEFAULTTONEAREST), &raw mut mi)
        SetWindowLongPtrW(w.hwnd, GWL_STYLE, (w.saved_style & ~((WS_CAPTION | WS_THICKFRAME) as LONG_PTR)) | (WS_POPUP as LONG_PTR))
        SetWindowPos(
            w.hwnd,
            HWND_TOPMOST,
            mi.rcMonitor.left,
            mi.rcMonitor.top,
            mi.rcMonitor.right - mi.rcMonitor.left,
            mi.rcMonitor.bottom - mi.rcMonitor.top,
            SWP_FRAMECHANGED | SWP_NOACTIVATE
        )
        w.fullscreen = true
    } else {
        SetWindowLongPtrW(w.hwnd, GWL_STYLE, w.saved_style)
        SetWindowPos(
            w.hwnd,
            HWND_NOTOPMOST,
            w.saved_rect.left,
            w.saved_rect.top,
            w.saved_rect.right - w.saved_rect.left,
            w.saved_rect.bottom - w.saved_rect.top,
            SWP_FRAMECHANGED | SWP_NOACTIVATE
        )
        w.fullscreen = false
    }
}

public func window_is_fullscreen(w : *mut Window) : bool {
    return w.fullscreen
}

public func window_maximize(w : *mut Window) {
    if(w.hwnd != null) {
        ShowWindow(w.hwnd, SW_MAXIMIZE)
    }
}

public func window_minimize(w : *mut Window) {
    if(w.hwnd != null) {
        ShowWindow(w.hwnd, SW_MINIMIZE)
    }
}

public func window_restore(w : *mut Window) {
    if(w.hwnd != null) {
        ShowWindow(w.hwnd, SW_RESTORE)
    }
}

public func window_show(w : *mut Window) {
    if(w.hwnd != null) {
        ShowWindow(w.hwnd, SW_SHOW)
        UpdateWindow(w.hwnd)
        w.visible = true
    }
}

public func window_hide(w : *mut Window) {
    if(w.hwnd != null) {
        ShowWindow(w.hwnd, SW_HIDE)
    }
    w.visible = false
}

public func window_focus(w : *mut Window) {
    if(w.hwnd != null) {
        SetForegroundWindow(w.hwnd)
        SetFocus(w.hwnd)
    }
}

public func window_close(w : *mut Window) {
    if(w.hwnd != null) {
        SendMessageW(w.hwnd, WM_CLOSE, 0, 0)
    }
}

// --- visual ---

public func window_set_cursor(w : *mut Window, cursor : int) {
    w.cursor = cursor
    SetCursor(cursor_for_kind(cursor))
}

public func window_set_icon(w : *mut Window, path : *char) {
    if(w.hwnd == null || path == null) {
        return
    }
    var wbuf : [1024]ushort
    widen_to_buf(path, &raw mut wbuf[0], 1024)
    var icon = LoadImageW(
        null,
        &raw wbuf[0],
        IMAGE_ICON,
        GetSystemMetrics(SM_CXICON),
        GetSystemMetrics(SM_CYICON),
        LR_LOADFROMFILE | LR_DEFAULTSIZE
    ) as HICON
    if(icon != null) {
        SendMessageW(w.hwnd, WM_SETICON, ICON_BIG, icon as LPARAM)
        SendMessageW(w.hwnd, WM_SETICON, ICON_SMALL, icon as LPARAM)
    }
}

public func window_set_opacity(w : *mut Window, opacity : double) {
    w.opacity = opacity
    if(w.hwnd == null) {
        return
    }
    var ex = GetWindowLongPtrW(w.hwnd, GWL_EXSTYLE)
    SetWindowLongPtrW(w.hwnd, GWL_EXSTYLE, ex | (WS_EX_LAYERED as LONG_PTR))
    var alpha = (opacity * 255.0) as int
    if(alpha < 0) {
        alpha = 0
    } else if(alpha > 255) {
        alpha = 255
    }
    win_set_layered_alpha(w.hwnd, alpha as BYTE)
}

public func window_set_always_on_top(w : *mut Window, on_top : bool) {
    w.always_on_top = on_top
    if(w.hwnd != null) {
        if(on_top) {
            SetWindowPos(w.hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE)
        } else {
            SetWindowPos(w.hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE)
        }
    }
}

public func window_set_decorated(w : *mut Window, decorated : bool) {
    if(w.hwnd == null || decorated == w.decorated) {
        return
    }
    var style = GetWindowLongPtrW(w.hwnd, GWL_STYLE)
    if(decorated) {
        style = (style & ~(WS_POPUP as LONG_PTR)) | (WS_OVERLAPPEDWINDOW as LONG_PTR)
    } else {
        style = (style & ~(WS_OVERLAPPEDWINDOW as LONG_PTR)) | (WS_POPUP as LONG_PTR)
    }
    SetWindowLongPtrW(w.hwnd, GWL_STYLE, style)
    SetWindowPos(w.hwnd, null, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED)
    w.decorated = decorated
}

// --- DPI / scale ---

public func window_dpi(w : *mut Window) : int {
    if(w.hwnd != null) {
        var dpi = GetDpiForWindow(w.hwnd) as int
        if(dpi > 0) {
            w.dpi = dpi
        }
    }
    return w.dpi
}

public func window_scale_factor(w : *mut Window) : double {
    return (window_dpi(w) as double) / 96.0
}

// --- monitor information ---

var g_monitor_count_scratch : int = 0
var g_monitor_index_scratch : int = 0
var g_monitor_rect_scratch : RECT = RECT { left : 0, top : 0, right : 0, bottom : 0 }

func win_monitor_enum_proc(hmon : HMONITOR, hdc : HDC, lprc : *mut RECT, data : LPARAM) : BOOL {
    var mi : MONITORINFO
    mi.cbSize = sizeof(MONITORINFO) as DWORD
    GetMonitorInfoW(hmon, &raw mut mi)
    if(g_monitor_index_scratch == g_monitor_count_scratch) {
        g_monitor_rect_scratch = mi.rcMonitor
    }
    g_monitor_count_scratch += 1
    return 1
}

public func window_monitor_count() : int {
    return GetSystemMetrics(SM_CMONITORS)
}

public func window_monitor_bounds(index : int) : Rect {
    g_monitor_count_scratch = 0
    g_monitor_index_scratch = index
    EnumDisplayMonitors(null, null, win_monitor_enum_proc, 0)
    return Rect.make(
        g_monitor_rect_scratch.left,
        g_monitor_rect_scratch.top,
        g_monitor_rect_scratch.right - g_monitor_rect_scratch.left,
        g_monitor_rect_scratch.bottom - g_monitor_rect_scratch.top
    )
}

public func window_monitor_scale(index : int) : double {
    return (win_get_dpi_for_system() as double) / 96.0
}

// --- callbacks ---

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

public func window_set_command_callback(w : *mut Window, cb : CommandCallback) {
    w.command_cb = cb
}

// --- native handle ---

// Returns the platform native handle: an HWND on Windows, a GtkWidget* on
// Linux. Use with platform-specific code; the handle stays valid while the
// window is created.
public func window_native_handle(w : *mut Window) : *mut void {
    return w.hwnd as *mut void
}

// --- message loop (thread-level) ---

public func window_run() {
    var msg : MSG
    while(GetMessageW(&raw mut msg, null, 0, 0) > 0) {
        TranslateMessage(&raw mut msg)
        DispatchMessageW(&raw mut msg)
    }
}

public func window_quit() {
    PostQuitMessage(0)
}

} // end namespace window
