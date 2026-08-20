// ---------------------------------------------------------------------------
// win.ch — Windows WebView2 backend for the Chemical webview library.
//
// Faithful Chemical port of lang/compiled/webview-c/wvwin.c (pure-C WebView2):
//   - top-level window + CHILD widget window (controller embedded in widget)
//   - com_handler object implementing env/ctrl/permission interfaces
//   - userDataFolder = %APPDATA%\<exe name>
//   - environment creation via WebView2Loader.dll (no DLLs are shipped next to
//     the app; the loader comes from the installed WebView2 runtime and is
//     found through the system search path)
//   - GetMessageW pump until the webview is ready
//
// The public API below (webview_*) matches the linux backend in
// posix/linux.ch exactly.
// ---------------------------------------------------------------------------

public namespace webview {

using std::string;

// ===========================================================================
// Win32 types (the ones cstd does not provide)
// ===========================================================================

public type HWND = HANDLE
public type HICON = HANDLE
public type HMENU = HANDLE
public type HBRUSH = HANDLE
public type HCURSOR = HANDLE

/** 32-bit COM status code (long in C). */
public type HRESULT = LONG
/** unsigned 32-bit (COM ref counts). */
public type ULONG = ulong
public type INT64 = bigint
/** window class atom (RegisterClassExW). */
public type ATOM = WORD

/** const IID& from the C headers — passed as a pointer on the ABI level. */
public type REFIID = *GUID

public type DPI_AWARENESS_CONTEXT = *mut void

public type WNDPROC = (hwnd : HWND, msg : UINT, wp : WPARAM, lp : LPARAM) => LRESULT

// The Win32 structs shared with the window library (RECT/MSG/POINT/... live in
// lang/libs/window/win/win.ch). Reusing them keeps the extern declarations of
// the common user32 functions identical across the two modules when they are
// compiled into the same C translation unit.
public type LPCREATESTRUCTW = *mut window::CREATESTRUCTW
public type LPMINMAXINFO = *mut window::MINMAXINFO
public type LPRECT = *mut window::RECT
public type POINT = window::POINT
public type RECT = window::RECT
public type MSG = window::MSG
public type MINMAXINFO = window::MINMAXINFO
public type CREATESTRUCTW = window::CREATESTRUCTW
public type WNDCLASSEXW = window::WNDCLASSEXW

// ===========================================================================
// WebView2-specific Win32 structs (not shared with the window library)
// ===========================================================================

public struct GUID {
    var Data1 : DWORD
    var Data2 : WORD
    var Data3 : WORD
    var Data4 : [8]BYTE
}

public type IID = GUID

public struct EventRegistrationToken {
    var value : INT64
}

// ===========================================================================
// Win32 constants
// ===========================================================================

public comptime const MAX_PATH : int = 260

// COM / OLE
public comptime const COINIT_APARTMENTTHREADED : DWORD = 0x2 as DWORD
public comptime const CSIDL_APPDATA : int = 0x001a

// DPI
public comptime const DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 : DPI_AWARENESS_CONTEXT = -4 as DPI_AWARENESS_CONTEXT

// HRESULT codes
public comptime const S_OK : HRESULT = 0
public comptime const E_ABORT : HRESULT = 0x80004004 as HRESULT
public comptime const E_FAIL : HRESULT = 0x80004005 as HRESULT
public comptime const E_POINTER : HRESULT = 0x80004003 as HRESULT
public comptime const E_NOINTERFACE : HRESULT = 0x80004002 as HRESULT

// Win32 error codes (HRESULT_FROM_WIN32 computed below)
public comptime const ERROR_MOD_NOT_FOUND : DWORD = 126 as DWORD
public comptime const ERROR_INVALID_STATE : DWORD = 1409 as DWORD
public comptime const ERROR_ACCESS_DENIED : DWORD = 5 as DWORD

// Window messages
public comptime const WM_NCCREATE : UINT = 0x0081 as UINT
public comptime const WM_NCDESTROY : UINT = 0x0082 as UINT
public comptime const WM_SIZE : UINT = 0x0005 as UINT
public comptime const WM_DESTROY : UINT = 0x0002 as UINT
public comptime const WM_CLOSE : UINT = 0x0010 as UINT
public comptime const WM_GETMINMAXINFO : UINT = 0x0024 as UINT
public comptime const WM_ACTIVATE : UINT = 0x0006 as UINT
public comptime const WM_QUIT : UINT = 0x0012 as UINT

// Window styles
public comptime const WS_OVERLAPPEDWINDOW : DWORD = 0x00CF0000 as DWORD
public comptime const WS_CHILD : DWORD = 0x40000000 as DWORD
public comptime const WS_EX_CONTROLPARENT : DWORD = 0x00010000 as DWORD
public comptime const WS_THICKFRAME : DWORD = 0x00040000 as DWORD
public comptime const WS_MAXIMIZEBOX : DWORD = 0x00010000 as DWORD

// GetWindowLongPtrW / SetWindowLongPtrW indexes
public comptime const GWL_STYLE : int = -16
public comptime const GWLP_USERDATA : int = -21
public comptime const GWLP_WNDPROC : int = -4

// ShowWindow / SetWindowPos
public comptime const SW_SHOW : int = 5
public comptime const SWP_NOSIZE : UINT = 0x0001 as UINT
public comptime const SWP_NOMOVE : UINT = 0x0002 as UINT
public comptime const SWP_NOZORDER : UINT = 0x0004 as UINT
public comptime const SWP_NOACTIVATE : UINT = 0x0010 as UINT
public comptime const SWP_FRAMECHANGED : UINT = 0x0020 as UINT

// CreateWindow position
public comptime const CW_USEDEFAULT : int = 0x80000000 as int

// LoadImage
public comptime const IDI_APPLICATION : LPCWSTR = 32512 as LPCWSTR
public comptime const IMAGE_ICON : UINT = 1 as UINT
public comptime const LR_DEFAULTCOLOR : UINT = 0x0000 as UINT
public comptime const SM_CXICON : int = 11
public comptime const SM_CYICON : int = 12

// COINIT / HRESULT helpers (plain functions: they are called with runtime
// values, so they must not be comptime-only)
public func SUCCEEDED(hr : HRESULT) : bool {
    return hr >= 0
}

public func FAILED(hr : HRESULT) : bool {
    return hr < 0
}

public func HRESULT_FROM_WIN32(err : DWORD) : HRESULT {
    var e = err as HRESULT
    if(e <= 0) {
        return e
    }
    return ((err & (0x0000FFFF as DWORD)) | ((7 as DWORD) << 16) | (0x80000000 as DWORD)) as HRESULT
}

// COREWEBVIEW2_PERMISSION_KIND (int so the vtable matches the C enum ABI)
public comptime const PERMISSION_KIND_UNKNOWN : int = 0
public comptime const PERMISSION_KIND_MICROPHONE : int = 1
public comptime const PERMISSION_KIND_CAMERA : int = 2
public comptime const PERMISSION_KIND_GEOLOCATION : int = 3
public comptime const PERMISSION_KIND_NOTIFICATIONS : int = 4
public comptime const PERMISSION_KIND_OTHER_SENSORS : int = 5
public comptime const PERMISSION_KIND_CLIPBOARD_READ : int = 6

// COREWEBVIEW2_PERMISSION_STATE
public comptime const PERMISSION_STATE_DEFAULT : int = 0
public comptime const PERMISSION_STATE_ALLOW : int = 1
public comptime const PERMISSION_STATE_DENY : int = 2

// COREWEBVIEW2_MOVE_FOCUS_REASON
public comptime const MOVE_FOCUS_REASON_PROGRAMMATIC : int = 0
public comptime const MOVE_FOCUS_REASON_NEXT : int = 1
public comptime const MOVE_FOCUS_REASON_PREVIOUS : int = 2

// webview_hint_t values (from webview/types.h)
public comptime const WV_HINT_NONE : int = 0
public comptime const WV_HINT_MIN : int = 1
public comptime const WV_HINT_MAX : int = 2
public comptime const WV_HINT_FIXED : int = 3

// ===========================================================================
// Win32 functions not provided by cstd
// ===========================================================================

// kernel32
@extern @stdcall @dllimport public func LoadLibraryW(lpLibFileName : LPCWSTR) : HMODULE
@extern @stdcall @dllimport public func GetModuleFileNameW(hModule : HMODULE, lpFilename : LPWSTR, nSize : DWORD) : DWORD
@extern @stdcall @dllimport public func GetModuleHandleW(lpModuleName : LPCWSTR) : HMODULE

// ole32 (COM)
@extern @stdcall @dllimport public func CoInitializeEx(pvReserved : LPVOID, dwCoInit : DWORD) : HRESULT
@extern @stdcall @dllimport public func CoUninitialize() : void
@extern @stdcall @dllimport public func CoTaskMemFree(pv : LPVOID) : void
// NOTE: the SDK header defines IsEqualIID as a macro expanding to IsEqualGUID;
// IsEqualGUID is the actual ole32 export, so that is what we bind to.
@extern @stdcall @dllimport public func IsEqualGUID(riid1 : REFIID, riid2 : REFIID) : BOOL

// shell32 (SHGetFolderPathW)
@extern @stdcall @dllimport public func SHGetFolderPathW(hwnd : HWND, csidl : int, hToken : HANDLE, dwFlags : DWORD, pszPath : LPWSTR) : HRESULT

// shlwapi
@extern @stdcall @dllimport public func PathCombineW(pszDest : LPWSTR, pszDir : LPCWSTR, pszFile : LPCWSTR) : LPWSTR
@extern @stdcall @dllimport public func PathFindFileNameW(pszPath : LPCWSTR) : LPCWSTR

// user32 (DPI)
@extern @stdcall @dllimport public func SetProcessDpiAwarenessContext(value : DPI_AWARENESS_CONTEXT) : BOOL
@extern @stdcall @dllimport public func EnableNonClientDpiScaling(hwnd : HWND) : BOOL
@extern @stdcall @dllimport public func GetDpiForWindow(hwnd : HWND) : UINT
@extern @stdcall @dllimport public func AdjustWindowRectExForDpi(lpRect : LPRECT, dwStyle : DWORD, bMenu : BOOL, dwExStyle : DWORD, dpi : UINT) : BOOL

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
@extern @stdcall @dllimport public func MoveWindow(hwnd : HWND, x : int, y : int, nWidth : int, nHeight : int, bRepaint : BOOL) : BOOL
@extern @stdcall @dllimport public func DestroyWindow(hwnd : HWND) : BOOL
@extern @stdcall @dllimport public func PostQuitMessage(nExitCode : int) : void
@extern @stdcall @dllimport public func GetMessageW(lpMsg : *mut MSG, hWnd : HWND, wMsgFilterMin : UINT, wMsgFilterMax : UINT) : BOOL
@extern @stdcall @dllimport public func TranslateMessage(lpMsg : *mut MSG) : BOOL
@extern @stdcall @dllimport public func DispatchMessageW(lpMsg : *mut MSG) : LRESULT
@extern @stdcall @dllimport public func ShowWindow(hwnd : HWND, nCmdShow : int) : BOOL
@extern @stdcall @dllimport public func UpdateWindow(hwnd : HWND) : BOOL
@extern @stdcall @dllimport public func SetFocus(hwnd : HWND) : HWND
@extern @stdcall @dllimport public func SetWindowPos(hwnd : HWND, hWndInsertAfter : HWND, x : int, y : int, cx : int, cy : int, uFlags : UINT) : BOOL
@extern @stdcall @dllimport public func SetForegroundWindow(hwnd : HWND) : BOOL
// window subclassing / properties (embed mode: webview in a section of an
// app-owned window; the parent window is subclassed so the webview stays
// pinned to its section across parent resizes)
@extern @stdcall @dllimport public func CallWindowProcW(prev : WNDPROC, hwnd : HWND, msg : UINT, wp : WPARAM, lp : LPARAM) : LRESULT
@extern @stdcall @dllimport public func SetPropW(hwnd : HWND, name : LPCWSTR, data : HANDLE) : BOOL
@extern @stdcall @dllimport public func GetPropW(hwnd : HWND, name : LPCWSTR) : HANDLE
@extern @stdcall @dllimport public func RemovePropW(hwnd : HWND, name : LPCWSTR) : HANDLE
// NOTE: the SDK header maps LoadImage -> LoadImageW (the real user32 export).
@extern @stdcall @dllimport public func LoadImageW(hInstance : HINSTANCE, name : LPCWSTR, type : UINT, cx : int, cy : int, fuLoad : UINT) : HANDLE
@extern @stdcall @dllimport public func GetSystemMetrics(nIndex : int) : int

// ===========================================================================
// WebView2 COM interface declarations (vtables per official WebView2.idl)
// ===========================================================================

// NOTE: the interface structs are forward-referenced by the vtables below;
// Chemical resolves types module-wide, so the full definitions that follow
// are sufficient (no separate forward declarations needed).

// ---- handler vtables ----

public struct ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandlerVtbl {
    var QueryInterface : (s : *mut ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler, riid : REFIID, ppv : *mut *mut void) => HRESULT
    var AddRef : (s : *mut ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler) => ULONG
    var Release : (s : *mut ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler) => ULONG
    var Invoke : (s : *mut ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler, res : HRESULT, env : *mut ICoreWebView2Environment) => HRESULT
}

public struct ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler {
    var lpVtbl : *ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandlerVtbl
}

public struct ICoreWebView2CreateCoreWebView2ControllerCompletedHandlerVtbl {
    var QueryInterface : (s : *mut ICoreWebView2CreateCoreWebView2ControllerCompletedHandler, riid : REFIID, ppv : *mut *mut void) => HRESULT
    var AddRef : (s : *mut ICoreWebView2CreateCoreWebView2ControllerCompletedHandler) => ULONG
    var Release : (s : *mut ICoreWebView2CreateCoreWebView2ControllerCompletedHandler) => ULONG
    var Invoke : (s : *mut ICoreWebView2CreateCoreWebView2ControllerCompletedHandler, res : HRESULT, controller : *mut ICoreWebView2Controller) => HRESULT
}

public struct ICoreWebView2CreateCoreWebView2ControllerCompletedHandler {
    var lpVtbl : *ICoreWebView2CreateCoreWebView2ControllerCompletedHandlerVtbl
}

public struct ICoreWebView2PermissionRequestedEventHandlerVtbl {
    var QueryInterface : (s : *mut ICoreWebView2PermissionRequestedEventHandler, riid : REFIID, ppv : *mut *mut void) => HRESULT
    var AddRef : (s : *mut ICoreWebView2PermissionRequestedEventHandler) => ULONG
    var Release : (s : *mut ICoreWebView2PermissionRequestedEventHandler) => ULONG
    var Invoke : (s : *mut ICoreWebView2PermissionRequestedEventHandler, sender : *mut ICoreWebView2, args : *mut ICoreWebView2PermissionRequestedEventArgs) => HRESULT
}

public struct ICoreWebView2PermissionRequestedEventHandler {
    var lpVtbl : *ICoreWebView2PermissionRequestedEventHandlerVtbl
}

// ---- ICoreWebView2ExecuteScriptCompletedHandler (evaluate_js result) ----

public struct ICoreWebView2ExecuteScriptCompletedHandlerVtbl {
    var QueryInterface : (s : *mut ICoreWebView2ExecuteScriptCompletedHandler, riid : REFIID, ppv : *mut *mut void) => HRESULT
    var AddRef : (s : *mut ICoreWebView2ExecuteScriptCompletedHandler) => ULONG
    var Release : (s : *mut ICoreWebView2ExecuteScriptCompletedHandler) => ULONG
    var Invoke : (s : *mut ICoreWebView2ExecuteScriptCompletedHandler, res : HRESULT, resultJson : LPCWSTR) => HRESULT
}

public struct ICoreWebView2ExecuteScriptCompletedHandler {
    var lpVtbl : *ICoreWebView2ExecuteScriptCompletedHandlerVtbl
}

// ---- ICoreWebView2Environment ----

public struct ICoreWebView2EnvironmentVtbl {
    var QueryInterface : (s : *mut ICoreWebView2Environment, riid : REFIID, ppv : *mut *mut void) => HRESULT
    var AddRef : (s : *mut ICoreWebView2Environment) => ULONG
    var Release : (s : *mut ICoreWebView2Environment) => ULONG
    var CreateCoreWebView2Controller : (s : *mut ICoreWebView2Environment, parent : HWND, handler : *mut ICoreWebView2CreateCoreWebView2ControllerCompletedHandler) => HRESULT
}

public struct ICoreWebView2Environment {
    var lpVtbl : *ICoreWebView2EnvironmentVtbl
}

// ---- ICoreWebView2Controller (get_CoreWebView2 = slot 25) ----

public struct ICoreWebView2ControllerVtbl {
    var QueryInterface : (s : *mut ICoreWebView2Controller, riid : REFIID, ppv : *mut *mut void) => HRESULT
    var AddRef : (s : *mut ICoreWebView2Controller) => ULONG
    var Release : (s : *mut ICoreWebView2Controller) => ULONG
    var get_IsVisible : (s : *mut ICoreWebView2Controller, value : *mut BOOL) => HRESULT
    var put_IsVisible : (s : *mut ICoreWebView2Controller, value : BOOL) => HRESULT
    var get_Bounds : (s : *mut ICoreWebView2Controller, value : *mut RECT) => HRESULT
    var put_Bounds : (s : *mut ICoreWebView2Controller, value : RECT) => HRESULT
    var get_ZoomFactor : (s : *mut ICoreWebView2Controller, value : *mut double) => HRESULT
    var put_ZoomFactor : (s : *mut ICoreWebView2Controller, value : double) => HRESULT
    var add_ZoomFactorChanged : (s : *mut ICoreWebView2Controller, handler : *mut void, token : *mut EventRegistrationToken) => HRESULT
    var remove_ZoomFactorChanged : (s : *mut ICoreWebView2Controller, token : EventRegistrationToken) => HRESULT
    var SetBoundsAndZoomFactor : (s : *mut ICoreWebView2Controller, bounds : RECT, zoom : double) => HRESULT
    var MoveFocus : (s : *mut ICoreWebView2Controller, reason : int) => HRESULT
    var add_MoveFocusRequested : (s : *mut ICoreWebView2Controller, handler : *mut void, token : *mut EventRegistrationToken) => HRESULT
    var remove_MoveFocusRequested : (s : *mut ICoreWebView2Controller, token : EventRegistrationToken) => HRESULT
    var add_GotFocus : (s : *mut ICoreWebView2Controller, handler : *mut void, token : *mut EventRegistrationToken) => HRESULT
    var remove_GotFocus : (s : *mut ICoreWebView2Controller, token : EventRegistrationToken) => HRESULT
    var add_LostFocus : (s : *mut ICoreWebView2Controller, handler : *mut void, token : *mut EventRegistrationToken) => HRESULT
    var remove_LostFocus : (s : *mut ICoreWebView2Controller, token : EventRegistrationToken) => HRESULT
    var add_AcceleratorKeyPressed : (s : *mut ICoreWebView2Controller, handler : *mut void, token : *mut EventRegistrationToken) => HRESULT
    var remove_AcceleratorKeyPressed : (s : *mut ICoreWebView2Controller, token : EventRegistrationToken) => HRESULT
    var get_ParentWindow : (s : *mut ICoreWebView2Controller, value : *mut HWND) => HRESULT
    var put_ParentWindow : (s : *mut ICoreWebView2Controller, value : HWND) => HRESULT
    var NotifyParentWindowPositionChanged : (s : *mut ICoreWebView2Controller) => HRESULT
    var Close : (s : *mut ICoreWebView2Controller) => HRESULT
    var get_CoreWebView2 : (s : *mut ICoreWebView2Controller, value : *mut *mut ICoreWebView2) => HRESULT
}

public struct ICoreWebView2Controller {
    var lpVtbl : *ICoreWebView2ControllerVtbl
}

// ---- ICoreWebView2 (slots 3..29) ----

public struct ICoreWebView2Vtbl {
    var QueryInterface : (s : *mut ICoreWebView2, riid : REFIID, ppv : *mut *mut void) => HRESULT
    var AddRef : (s : *mut ICoreWebView2) => ULONG
    var Release : (s : *mut ICoreWebView2) => ULONG
    var get_Settings : (s : *mut ICoreWebView2, value : *mut *mut ICoreWebView2Settings) => HRESULT
    var get_Source : (s : *mut ICoreWebView2, value : *mut LPWSTR) => HRESULT
    var Navigate : (s : *mut ICoreWebView2, uri : LPCWSTR) => HRESULT
    var NavigateToString : (s : *mut ICoreWebView2, html : LPCWSTR) => HRESULT
    var add_NavigationStarting : (s : *mut ICoreWebView2, handler : *mut void, token : *mut EventRegistrationToken) => HRESULT
    var remove_NavigationStarting : (s : *mut ICoreWebView2, token : EventRegistrationToken) => HRESULT
    var add_ContentLoading : (s : *mut ICoreWebView2, handler : *mut void, token : *mut EventRegistrationToken) => HRESULT
    var remove_ContentLoading : (s : *mut ICoreWebView2, token : EventRegistrationToken) => HRESULT
    var add_SourceChanged : (s : *mut ICoreWebView2, handler : *mut void, token : *mut EventRegistrationToken) => HRESULT
    var remove_SourceChanged : (s : *mut ICoreWebView2, token : EventRegistrationToken) => HRESULT
    var add_HistoryChanged : (s : *mut ICoreWebView2, handler : *mut void, token : *mut EventRegistrationToken) => HRESULT
    var remove_HistoryChanged : (s : *mut ICoreWebView2, token : EventRegistrationToken) => HRESULT
    var add_NavigationCompleted : (s : *mut ICoreWebView2, handler : *mut void, token : *mut EventRegistrationToken) => HRESULT
    var remove_NavigationCompleted : (s : *mut ICoreWebView2, token : EventRegistrationToken) => HRESULT
    var add_FrameNavigationStarting : (s : *mut ICoreWebView2, handler : *mut void, token : *mut EventRegistrationToken) => HRESULT
    var remove_FrameNavigationStarting : (s : *mut ICoreWebView2, token : EventRegistrationToken) => HRESULT
    var add_FrameNavigationCompleted : (s : *mut ICoreWebView2, handler : *mut void, token : *mut EventRegistrationToken) => HRESULT
    var remove_FrameNavigationCompleted : (s : *mut ICoreWebView2, token : EventRegistrationToken) => HRESULT
    var add_ScriptDialogOpening : (s : *mut ICoreWebView2, handler : *mut void, token : *mut EventRegistrationToken) => HRESULT
    var remove_ScriptDialogOpening : (s : *mut ICoreWebView2, token : EventRegistrationToken) => HRESULT
    var add_PermissionRequested : (s : *mut ICoreWebView2, handler : *mut void, token : *mut EventRegistrationToken) => HRESULT
    var remove_PermissionRequested : (s : *mut ICoreWebView2, token : EventRegistrationToken) => HRESULT
    var add_ProcessFailed : (s : *mut ICoreWebView2, handler : *mut void, token : *mut EventRegistrationToken) => HRESULT
    var remove_ProcessFailed : (s : *mut ICoreWebView2, token : EventRegistrationToken) => HRESULT
    var AddScriptToExecuteOnDocumentCreated : (s : *mut ICoreWebView2, javaScript : LPCWSTR, handler : *mut void) => HRESULT
    var RemoveScriptToExecuteOnDocumentCreated : (s : *mut ICoreWebView2, id : *mut void) => HRESULT
    var ExecuteScript : (s : *mut ICoreWebView2, javaScript : LPCWSTR, handler : *mut void) => HRESULT
}

public struct ICoreWebView2 {
    var lpVtbl : *ICoreWebView2Vtbl
}

// ---- ICoreWebView2Settings ----

public struct ICoreWebView2SettingsVtbl {
    var QueryInterface : (s : *mut ICoreWebView2Settings, riid : REFIID, ppv : *mut *mut void) => HRESULT
    var AddRef : (s : *mut ICoreWebView2Settings) => ULONG
    var Release : (s : *mut ICoreWebView2Settings) => ULONG
    var get_IsScriptEnabled : (s : *mut ICoreWebView2Settings, value : *mut BOOL) => HRESULT
    var put_IsScriptEnabled : (s : *mut ICoreWebView2Settings, value : BOOL) => HRESULT
    var get_IsWebMessageEnabled : (s : *mut ICoreWebView2Settings, value : *mut BOOL) => HRESULT
    var put_IsWebMessageEnabled : (s : *mut ICoreWebView2Settings, value : BOOL) => HRESULT
    var get_AreDefaultScriptDialogsEnabled : (s : *mut ICoreWebView2Settings, value : *mut BOOL) => HRESULT
    var put_AreDefaultScriptDialogsEnabled : (s : *mut ICoreWebView2Settings, value : BOOL) => HRESULT
    var get_IsStatusBarEnabled : (s : *mut ICoreWebView2Settings, value : *mut BOOL) => HRESULT
    var put_IsStatusBarEnabled : (s : *mut ICoreWebView2Settings, value : BOOL) => HRESULT
    var get_AreDevToolsEnabled : (s : *mut ICoreWebView2Settings, value : *mut BOOL) => HRESULT
    var put_AreDevToolsEnabled : (s : *mut ICoreWebView2Settings, value : BOOL) => HRESULT
}

public struct ICoreWebView2Settings {
    var lpVtbl : *ICoreWebView2SettingsVtbl
}

// ---- ICoreWebView2PermissionRequestedEventArgs ----

public struct ICoreWebView2PermissionRequestedEventArgsVtbl {
    var QueryInterface : (s : *mut ICoreWebView2PermissionRequestedEventArgs, riid : REFIID, ppv : *mut *mut void) => HRESULT
    var AddRef : (s : *mut ICoreWebView2PermissionRequestedEventArgs) => ULONG
    var Release : (s : *mut ICoreWebView2PermissionRequestedEventArgs) => ULONG
    var get_Uri : (s : *mut ICoreWebView2PermissionRequestedEventArgs, value : *mut LPWSTR) => HRESULT
    var get_PermissionKind : (s : *mut ICoreWebView2PermissionRequestedEventArgs, value : *mut int) => HRESULT
    var get_IsUserInitiated : (s : *mut ICoreWebView2PermissionRequestedEventArgs, value : *mut BOOL) => HRESULT
    var get_State : (s : *mut ICoreWebView2PermissionRequestedEventArgs, value : *mut int) => HRESULT
    var put_State : (s : *mut ICoreWebView2PermissionRequestedEventArgs, value : int) => HRESULT
}

public struct ICoreWebView2PermissionRequestedEventArgs {
    var lpVtbl : *ICoreWebView2PermissionRequestedEventArgsVtbl
}

// ===========================================================================
// IIDs (from Microsoft's WebView2 SDK header)
// ===========================================================================

public const IID_CTRL_COMPLETED : IID = IID {
    Data1 : 0x6C4819F3 as DWORD,
    Data2 : 0xC9B7 as WORD,
    Data3 : 0x4260 as WORD,
    Data4 : [0x81 as BYTE, 0x27 as BYTE, 0xC9 as BYTE, 0xF5 as BYTE, 0xBD as BYTE, 0xE7 as BYTE, 0xF6 as BYTE, 0x8C as BYTE]
}

public const IID_ENV_COMPLETED : IID = IID {
    Data1 : 0x4E8A3389 as DWORD,
    Data2 : 0xC9D8 as WORD,
    Data3 : 0x4BD2 as WORD,
    Data4 : [0xB6 as BYTE, 0xB5 as BYTE, 0x12 as BYTE, 0x4F as BYTE, 0xEE as BYTE, 0x6C as BYTE, 0xC1 as BYTE, 0x4D as BYTE]
}

public const IID_PERM_REQUESTED : IID = IID {
    Data1 : 0x15E1C6A3 as DWORD,
    Data2 : 0xC72A as WORD,
    Data3 : 0x4DF3 as WORD,
    Data4 : [0x91 as BYTE, 0xD7 as BYTE, 0xD0 as BYTE, 0x97 as BYTE, 0xFB as BYTE, 0xEC as BYTE, 0x6B as BYTE, 0xFD as BYTE]
}

// ===========================================================================
// loader: WebView2Loader.dll
// ===========================================================================

public type CreateEnvWithOptionsFn = (browser_dir : LPCWSTR, user_data_dir : LPCWSTR, environmentOptions : *mut void, handler : *mut ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler) => HRESULT
public type GetBrowserVersionFn = (browser_dir : LPCWSTR, version : *mut LPWSTR) => HRESULT

func loader_ensure(lib : *mut HMODULE) : HMODULE {
    if(*lib == null) {
        var wname : [64]ushort
        widen_to_buf("WebView2Loader.dll", &raw mut wname[0], 64)
        *lib = LoadLibraryW(&raw wname[0] as LPCWSTR)
    }
    return *lib
}

func loader_create_environment(
    lib : *mut HMODULE,
    browser_dir : LPCWSTR,
    user_data_dir : LPCWSTR,
    env_options : *mut void,
    handler : *mut ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler
) : HRESULT {
    var m = loader_ensure(lib)
    if(m == null) {
        return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND)
    }
    var fn = GetProcAddress(m, "CreateCoreWebView2EnvironmentWithOptions") as CreateEnvWithOptionsFn
    if(fn == null) {
        return E_FAIL
    }
    return fn(browser_dir, user_data_dir, env_options, handler)
}

func loader_get_browser_version(lib : *mut HMODULE, browser_dir : LPCWSTR, version : *mut LPWSTR) : HRESULT {
    var m = loader_ensure(lib)
    if(m == null) {
        return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND)
    }
    var fn = GetProcAddress(m, "GetAvailableCoreWebView2BrowserVersionString") as GetBrowserVersionFn
    if(fn == null) {
        return E_FAIL
    }
    return fn(browser_dir, version)
}

// ===========================================================================
// com_handler: one object implementing env + ctrl + permission interfaces
// ===========================================================================

public type HandlerCb = (ctx : *mut void, controller : *mut ICoreWebView2Controller, webview : *mut ICoreWebView2) => void
public type AttemptHandlerFn = (ctx : *mut void) => HRESULT

public struct Webview2ComHandler {
    var env_vtbl : *ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandlerVtbl
    var ctrl_vtbl : *ICoreWebView2CreateCoreWebView2ControllerCompletedHandlerVtbl
    var perm_vtbl : *ICoreWebView2PermissionRequestedEventHandlerVtbl
    var window : HWND
    var cb : HandlerCb
    var cb_ctx : *mut void
    var attempt_handler : AttemptHandlerFn
    var attempt_ctx : *mut void
    var ref_count : ULONG
    var attempts : uint
    var max_attempts : uint
    var sleep_ms : uint
}

// env_vtbl is member 0, so the env interface pointer IS the handler pointer.
func handler_from_env(s : *mut ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler) : *mut Webview2ComHandler {
    return s as *mut Webview2ComHandler
}

func handler_from_ctrl(s : *mut ICoreWebView2CreateCoreWebView2ControllerCompletedHandler) : *mut Webview2ComHandler {
    // the ctrl interface pointer is the address of the ctrl_vtbl field
    var base = s as *mut char
    return (base - (offsetof(Webview2ComHandler, ctrl_vtbl) as bigint)) as *mut Webview2ComHandler
}

func handler_from_perm(s : *mut ICoreWebView2PermissionRequestedEventHandler) : *mut Webview2ComHandler {
    // the perm interface pointer is the address of the perm_vtbl field
    var base = s as *mut char
    return (base - (offsetof(Webview2ComHandler, perm_vtbl) as bigint)) as *mut Webview2ComHandler
}

func handler_qi(h : *mut Webview2ComHandler, riid : REFIID, ppv : *mut *mut void) : HRESULT {
    if(ppv == null) {
        return E_POINTER
    }
    if(IsEqualGUID(riid, &raw IID_CTRL_COMPLETED)) {
        *ppv = &raw h.ctrl_vtbl as *mut void
        h.ref_count += 1
        return S_OK
    }
    if(IsEqualGUID(riid, &raw IID_ENV_COMPLETED)) {
        *ppv = &raw h.env_vtbl as *mut void
        h.ref_count += 1
        return S_OK
    }
    if(IsEqualGUID(riid, &raw IID_PERM_REQUESTED)) {
        *ppv = &raw h.perm_vtbl as *mut void
        h.ref_count += 1
        return S_OK
    }
    *ppv = null
    return E_NOINTERFACE
}

func env_addref(s : *mut ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler) : ULONG {
    var h = handler_from_env(s)
    h.ref_count += 1
    return h.ref_count
}

func env_release(s : *mut ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler) : ULONG {
    var h = handler_from_env(s)
    if(h.ref_count > 1) {
        h.ref_count -= 1
        return h.ref_count
    }
    free(h as *mut void)
    return 0
}

func env_qi(s : *mut ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler, riid : REFIID, ppv : *mut *mut void) : HRESULT {
    return handler_qi(handler_from_env(s), riid, ppv)
}

func env_invoke(s : *mut ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler, res : HRESULT, env : *mut ICoreWebView2Environment) : HRESULT {
    var h = handler_from_env(s)
    if(SUCCEEDED(res)) {
        res = env.lpVtbl.CreateCoreWebView2Controller(
            env,
            h.window,
            &raw h.ctrl_vtbl as *mut ICoreWebView2CreateCoreWebView2ControllerCompletedHandler
        )
        if(SUCCEEDED(res)) {
            return S_OK
        }
    }
    handler_try_create_environment(h)
    return S_OK
}

func ctrl_addref(s : *mut ICoreWebView2CreateCoreWebView2ControllerCompletedHandler) : ULONG {
    var h = handler_from_ctrl(s)
    h.ref_count += 1
    return h.ref_count
}

func ctrl_release(s : *mut ICoreWebView2CreateCoreWebView2ControllerCompletedHandler) : ULONG {
    var h = handler_from_ctrl(s)
    if(h.ref_count > 1) {
        h.ref_count -= 1
        return h.ref_count
    }
    free(h as *mut void)
    return 0
}

func ctrl_qi(s : *mut ICoreWebView2CreateCoreWebView2ControllerCompletedHandler, riid : REFIID, ppv : *mut *mut void) : HRESULT {
    return handler_qi(handler_from_ctrl(s), riid, ppv)
}

func ctrl_invoke(s : *mut ICoreWebView2CreateCoreWebView2ControllerCompletedHandler, res : HRESULT, controller : *mut ICoreWebView2Controller) : HRESULT {
    var h = handler_from_ctrl(s)
    if(FAILED(res)) {
        // See handler_try_create_environment() regarding
        // HRESULT_FROM_WIN32(ERROR_INVALID_STATE).
        // The result is E_ABORT if the parent window has been destroyed already.
        if(res == HRESULT_FROM_WIN32(ERROR_INVALID_STATE) || res == E_ABORT) {
            return S_OK
        }
        handler_try_create_environment(h)
        return S_OK
    }

    var webview : *mut ICoreWebView2 = null
    var token : EventRegistrationToken
    controller.lpVtbl.get_CoreWebView2(controller, &raw mut webview)
    webview.lpVtbl.add_PermissionRequested(
        webview,
        &raw h.perm_vtbl as *mut ICoreWebView2PermissionRequestedEventHandler,
        &raw mut token
    )

    h.cb(h.cb_ctx, controller, webview)
    return S_OK
}

func perm_addref(s : *mut ICoreWebView2PermissionRequestedEventHandler) : ULONG {
    var h = handler_from_perm(s)
    h.ref_count += 1
    return h.ref_count
}

func perm_release(s : *mut ICoreWebView2PermissionRequestedEventHandler) : ULONG {
    var h = handler_from_perm(s)
    if(h.ref_count > 1) {
        h.ref_count -= 1
        return h.ref_count
    }
    free(h as *mut void)
    return 0
}

func perm_qi(s : *mut ICoreWebView2PermissionRequestedEventHandler, riid : REFIID, ppv : *mut *mut void) : HRESULT {
    return handler_qi(handler_from_perm(s), riid, ppv)
}

func perm_invoke(s : *mut ICoreWebView2PermissionRequestedEventHandler, sender : *mut ICoreWebView2, args : *mut ICoreWebView2PermissionRequestedEventArgs) : HRESULT {
    var kind : int = 0
    args.lpVtbl.get_PermissionKind(args, &raw mut kind)
    if(kind == PERMISSION_KIND_CLIPBOARD_READ) {
        args.lpVtbl.put_State(args, PERMISSION_STATE_ALLOW)
    }
    return S_OK
}

public const g_env_vtbl : ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandlerVtbl = ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandlerVtbl {
    QueryInterface : env_qi,
    AddRef : env_addref,
    Release : env_release,
    Invoke : env_invoke
}

public const g_ctrl_vtbl : ICoreWebView2CreateCoreWebView2ControllerCompletedHandlerVtbl = ICoreWebView2CreateCoreWebView2ControllerCompletedHandlerVtbl {
    QueryInterface : ctrl_qi,
    AddRef : ctrl_addref,
    Release : ctrl_release,
    Invoke : ctrl_invoke
}

public const g_perm_vtbl : ICoreWebView2PermissionRequestedEventHandlerVtbl = ICoreWebView2PermissionRequestedEventHandlerVtbl {
    QueryInterface : perm_qi,
    AddRef : perm_addref,
    Release : perm_release,
    Invoke : perm_invoke
}

// Retry creating a WebView2 environment.
func handler_try_create_environment(h : *mut Webview2ComHandler) {
    // WebView creation fails with HRESULT_FROM_WIN32(ERROR_INVALID_STATE) if a
    // running instance using the same user data folder exists, and the
    // Environment objects have different EnvironmentOptions.
    if(h.attempts < h.max_attempts) {
        h.attempts += 1
        var res = h.attempt_handler(h.attempt_ctx)
        if(SUCCEEDED(res)) {
            return
        }
        if(res == HRESULT_FROM_WIN32(ERROR_INVALID_STATE)) {
            return
        }
        Sleep(h.sleep_ms)
        handler_try_create_environment(h)
        return
    }
    // Give up.
    h.cb(h.cb_ctx, null, null)
}

// ===========================================================================
// the webview engine
// ===========================================================================

@direct_init
public struct WebView {
    // --- top-level window (standalone mode), owned by the window library
    //     (lang/libs/window). The webview widget fills its client area; the
    //     app can mix native UI with the webview through this window. ---
    var win : window::Window

    // --- WebView2 backend state (pure-C port) ---
    var widget : HWND
    var webview : *mut ICoreWebView2
    var controller : *mut ICoreWebView2Controller
    var handler : *mut Webview2ComHandler
    var loader_lib : HMODULE

    // --- embed mode (webview_attach): the webview lives in a section of an
    //     app-owned window::Window instead of creating its own top-level
    //     window. wv.win stays uncreated; wv.widget is a child of the parent's
    //     native handle positioned at `bounds` in the parent's client
    //     coordinates. The parent window is subclassed (wv_embed_proc) so the
    //     section tracks parent resizes. ---
    var attached : bool
    var parent : *mut window::Window
    var parent_wndproc : LONG_PTR // original parent wndproc, restored on teardown
    var bounds : RECT

    // --- cross-platform API fields (must match posix/linux.ch) ---
    var title : string
    var width : int
    var height : int
    var visible : bool
    var initialized : bool

    @make
    func make() : WebView {
        return WebView {
            win : window::Window.make(),
            widget : null,
            webview : null,
            controller : null,
            handler : null,
            loader_lib : null,
            attached : false,
            parent : null,
            parent_wndproc : 0,
            bounds : RECT { left : 0, top : 0, right : 0, bottom : 0 },
            title : string("Chemical WebView"),
            width : 800,
            height : 600,
            visible : false,
            initialized : false
        }
    }
}

// ---- window procedures ----

func wv_widget_proc(hwnd : HWND, msg : UINT, wp : WPARAM, lp : LPARAM) : LRESULT {
    var wv = GetWindowLongPtrW(hwnd, GWLP_USERDATA) as *mut WebView
    if(msg == WM_NCCREATE) {
        var cs = lp as *mut CREATESTRUCTW
        wv = cs.lpCreateParams as *mut WebView
        wv.widget = hwnd
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, wv as LONG_PTR)
        return 1
    }
    if(wv == null) {
        return DefWindowProcW(hwnd, msg, wp, lp)
    }
    switch(msg) {
        WM_SIZE => {
            wv_resize_webview(wv)
        }
        WM_DESTROY => {
            wv.widget = null
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0)
        }
        default => {
            return DefWindowProcW(hwnd, msg, wp, lp)
        }
    }
    return 0
}

// ---- standalone window callbacks (the top-level window lives in the window
//      library; these keep the widget filling its client area and forward
//      focus to the webview) ----

func wv_win_resize_cb(data : *mut void, width : int, height : int) {
    var wv = data as *mut WebView
    if(wv != null && wv.widget != null) {
        MoveWindow(wv.widget, 0, 0, width, height, 1)
        wv_resize_webview(wv)
    }
}

func wv_win_focus_cb(data : *mut void, focused : bool) {
    var wv = data as *mut WebView
    if(wv != null && focused && wv.controller != null) {
        wv.controller.lpVtbl.MoveFocus(wv.controller, MOVE_FOCUS_REASON_PROGRAMMATIC)
    }
}

// ---- embed mode: subclassing the app-owned parent window ----

// Window property used to associate a WebView with the parent window it is
// embedded into. The name must stay valid for the lifetime of the window
// (SetPropW does not copy it), so it lives in a module-level buffer that is
// filled once before the first attach.
var g_embed_prop_name : [32]ushort
var g_embed_prop_ready : int = 0

func wv_ensure_embed_prop_name() {
    if(g_embed_prop_ready == 0) {
        widen_to_buf("chem_webview_embed", &raw mut g_embed_prop_name[0], 32)
        g_embed_prop_ready = 1
    }
}

// Subclass window proc installed on the embed parent window. It keeps the
// webview widget pinned to its section (wv.bounds) when the parent is resized
// and, on WM_NCDESTROY, restores the parent's original window proc and removes
// the webview property before forwarding the message.
func wv_embed_proc(hwnd : HWND, msg : UINT, wp : WPARAM, lp : LPARAM) : LRESULT {
    var wv = GetPropW(hwnd, &raw g_embed_prop_name[0]) as *mut WebView
    if(wv == null) {
        return DefWindowProcW(hwnd, msg, wp, lp)
    }
    if(msg == WM_NCDESTROY) {
        var orig = wv.parent_wndproc
        SetWindowLongPtrW(hwnd, GWLP_WNDPROC, orig)
        RemovePropW(hwnd, &raw g_embed_prop_name[0])
        wv.attached = false
        wv.parent = null
        wv.parent_wndproc = 0
        if(orig != 0) {
            return CallWindowProcW(orig as WNDPROC, hwnd, msg, wp, lp)
        }
        return 0
    }
    switch(msg) {
        WM_SIZE => {
            // Re-pin the widget to the stored section. This runs before the
            // app's own WM_SIZE handler (subclass -> CallWindowProcW), so when
            // the app also calls webview_set_bounds the widget is moved twice
            // per resize — the second move wins and both are idempotent.
            if(wv.widget != null) {
                MoveWindow(
                    wv.widget,
                    wv.bounds.left,
                    wv.bounds.top,
                    wv.bounds.right - wv.bounds.left,
                    wv.bounds.bottom - wv.bounds.top,
                    1
                )
            }
        }
        default => {}
    }
    if(wv.parent_wndproc != 0) {
        return CallWindowProcW(wv.parent_wndproc as WNDPROC, hwnd, msg, wp, lp)
    }
    return DefWindowProcW(hwnd, msg, wp, lp)
}

func wv_attach_subclass(wv : *mut WebView) {
    wv_ensure_embed_prop_name()
    var parent_hwnd = wv.parent.hwnd
    wv.parent_wndproc = GetWindowLongPtrW(parent_hwnd, GWLP_WNDPROC)
    SetPropW(parent_hwnd, &raw g_embed_prop_name[0], wv as HANDLE)
    SetWindowLongPtrW(parent_hwnd, GWLP_WNDPROC, wv_embed_proc as LONG_PTR)
}

// ---- embed context + callbacks ----

public struct EmbedCtx {
    var wv : *mut WebView
    var handler : *mut Webview2ComHandler
    var userDataFolder : [260]ushort
    var done : int
}

func embed_acquired_cb(ctx : *mut void, controller : *mut ICoreWebView2Controller, webview : *mut ICoreWebView2) {
    var ec = ctx as *mut EmbedCtx
    if(controller == null || webview == null) {
        ec.done = 1
        return
    }
    controller.lpVtbl.AddRef(controller)
    webview.lpVtbl.AddRef(webview)
    ec.wv.controller = controller
    ec.wv.webview = webview
    ec.done = 1
}

func embed_attempt_handler(ctx : *mut void) : HRESULT {
    var ec = ctx as *mut EmbedCtx
    return loader_create_environment(
        &raw mut ec.wv.loader_lib,
        null,
        &raw ec.userDataFolder[0],
        null,
        &raw ec.handler.env_vtbl as *mut ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler
    )
}

// UTF-8 narrow string -> UTF-16 buffer (truncated safely if too small).
func widen_to_buf(utf8 : *char, buf : *mut ushort, cap : size_t) {
    if(buf == null || cap == 0) {
        return
    }
    var n = MultiByteToWideChar(65001, 0, utf8, -1, buf, cap as int) // CP_UTF8
    if(n <= 0) {
        buf[0] = 0
    }
}

func wv_embed(wv : *mut WebView, debug : int) : int {
    var ec = zeroed<EmbedCtx>()
    ec.wv = wv

    var currentExePath : [260]ushort
    GetModuleFileNameW(null, &raw mut currentExePath[0], 260)
    var currentExeName = PathFindFileNameW(&raw currentExePath[0])

    var dataPath : [260]ushort
    if(FAILED(SHGetFolderPathW(null, CSIDL_APPDATA, null, 0, &raw mut dataPath[0]))) {
        return -4
    }
    PathCombineW(&raw mut ec.userDataFolder[0], &raw dataPath[0], currentExeName)

    wv.handler = malloc(sizeof(Webview2ComHandler)) as *mut Webview2ComHandler
    if(wv.handler == null) {
        return -5
    }
    wv.handler.env_vtbl = &raw g_env_vtbl
    wv.handler.ctrl_vtbl = &raw g_ctrl_vtbl
    wv.handler.perm_vtbl = &raw g_perm_vtbl
    wv.handler.window = wv.widget
    wv.handler.cb = embed_acquired_cb
    wv.handler.cb_ctx = &raw mut ec as *mut void
    wv.handler.attempt_handler = null
    wv.handler.attempt_ctx = null
    wv.handler.ref_count = 1
    wv.handler.attempts = 0
    wv.handler.max_attempts = 60
    wv.handler.sleep_ms = 200
    ec.handler = wv.handler

    wv.handler.attempt_handler = embed_attempt_handler
    wv.handler.attempt_ctx = &raw mut ec as *mut void
    handler_try_create_environment(wv.handler)

    // Pump the message loop until WebView2 has finished initialization.
    var got_quit_msg : int = 0
    var msg : MSG
    while(ec.done == 0 && GetMessageW(&raw mut msg, null, 0, 0) >= 0) {
        if(msg.message == WM_QUIT) {
            got_quit_msg = 1
            break
        }
        TranslateMessage(&raw mut msg)
        DispatchMessageW(&raw mut msg)
    }
    if(got_quit_msg != 0) {
        return -6
    }
    if(wv.controller == null || wv.webview == null) {
        return -7
    }

    var settings : *mut ICoreWebView2Settings = null
    var res = wv.webview.lpVtbl.get_Settings(wv.webview, &raw mut settings)
    if(res != S_OK || settings == null) {
        return -8
    }
    var devtools_enabled : BOOL = 0
    if(debug != 0) {
        devtools_enabled = 1
    }
    res = settings.lpVtbl.put_AreDevToolsEnabled(settings, devtools_enabled)
    if(res != S_OK) {
        return -9
    }
    res = settings.lpVtbl.put_IsStatusBarEnabled(settings, 0)
    if(res != S_OK) {
        return -10
    }

    wv_resize_webview(wv)
    wv.controller.lpVtbl.put_IsVisible(wv.controller, 1)
    ShowWindow(wv.widget, SW_SHOW)
    UpdateWindow(wv.widget)
    if(wv.controller != null) {
        wv.controller.lpVtbl.MoveFocus(wv.controller, MOVE_FOCUS_REASON_PROGRAMMATIC)
    }
    return 0
}

func wv_resize_webview(wv : *mut WebView) {
    if(wv.widget != null && wv.controller != null) {
        var bounds : RECT
        if(GetClientRect(wv.widget, &raw mut bounds)) {
            wv.controller.lpVtbl.put_Bounds(wv.controller, bounds)
        }
    }
}

// ===========================================================================
// evaluate_js result handler (ICoreWebView2ExecuteScriptCompletedHandler)
// ===========================================================================

// One-shot COM object handed to ExecuteScript for webview_evaluate_js_result.
// The vtbl is member 0, so the interface pointer IS the struct pointer.
public struct JsEvalHandler {
    var vtbl : *ICoreWebView2ExecuteScriptCompletedHandlerVtbl
    var cb : JsResultCallback
    var user_data : *mut void
    var ref_count : ULONG
}

func js_handler_from_iface(s : *mut ICoreWebView2ExecuteScriptCompletedHandler) : *mut JsEvalHandler {
    return s as *mut JsEvalHandler
}

// Convert a UTF-16 result string to a malloc'd UTF-8 copy (caller frees).
func js_result_to_utf8(wide : LPCWSTR) : *mut char {
    if(wide == null) {
        return null
    }
    var needed = WideCharToMultiByte(65001, 0, wide as *u16, -1, null, 0, null, null)
    if(needed <= 0) {
        return null
    }
    var buf = malloc((needed + 1) as size_t) as *mut char
    if(buf == null) {
        return null
    }
    WideCharToMultiByte(65001, 0, wide as *u16, -1, buf, needed, null, null)
    return buf
}

func js_qi(s : *mut ICoreWebView2ExecuteScriptCompletedHandler, riid : REFIID, ppv : *mut *mut void) : HRESULT {
    if(ppv == null) {
        return E_POINTER
    }
    var h = js_handler_from_iface(s)
    *ppv = s as *mut void
    h.ref_count += 1
    return S_OK
}

func js_addref(s : *mut ICoreWebView2ExecuteScriptCompletedHandler) : ULONG {
    var h = js_handler_from_iface(s)
    h.ref_count += 1
    return h.ref_count
}

func js_release(s : *mut ICoreWebView2ExecuteScriptCompletedHandler) : ULONG {
    var h = js_handler_from_iface(s)
    if(h.ref_count > 1) {
        h.ref_count -= 1
        return h.ref_count
    }
    free(h as *mut void)
    return 0
}

func js_invoke(s : *mut ICoreWebView2ExecuteScriptCompletedHandler, res : HRESULT, resultJson : LPCWSTR) : HRESULT {
    var h = js_handler_from_iface(s)
    if(h.cb != null) {
        if(SUCCEEDED(res) && resultJson != null) {
            var utf8 = js_result_to_utf8(resultJson)
            if(utf8 != null) {
                h.cb(h.user_data, utf8)
                free(utf8 as *mut void)
            } else {
                h.cb(h.user_data, null)
            }
        } else {
            h.cb(h.user_data, null)
        }
    }
    return S_OK
}

public const g_js_vtbl : ICoreWebView2ExecuteScriptCompletedHandlerVtbl = ICoreWebView2ExecuteScriptCompletedHandlerVtbl {
    QueryInterface : js_qi,
    AddRef : js_addref,
    Release : js_release,
    Invoke : js_invoke
}

// ===========================================================================
// public API (matches posix/linux.ch exactly)
// ===========================================================================

public func webview_create(wv : *mut WebView) : std::Result<std::Unit, WebViewError> {
    var err = wv_init(wv, 0)
    if(err != 0) {
        var msg = string("")
        switch(err) {
            -1 => msg = string("WebView2 runtime not found")
            -2 => msg = string("failed to create top-level window")
            -3 => msg = string("failed to create widget window")
            -4 => msg = string("failed to get app data folder")
            -5 => msg = string("out of memory")
            -6 => msg = string("message loop quit during initialization")
            -7 => msg = string("webview controller was not acquired")
            -8 => msg = string("failed to get webview settings")
            default => {
                msg = string("webview init failed with code ")
                msg.append_integer(err as bigint)
            }
        }
        // Clean up any partially-created state (webview_destroy is null-safe).
        webview_destroy(wv)
        return std::Result.Err(WebViewError.InitFailed(msg))
    }
    wv.initialized = true
    return std::Result.Ok(std::Unit{})
}

// ===========================================================================
// public API: embed the webview into a section of an existing app window
// ===========================================================================

// Attach the webview to an app-owned window::Window instead of creating its
// own top-level window. The webview occupies the section (x, y, width, height)
// in the parent window's client coordinates; native UI can live in the rest of
// the window (add controls to the parent window through the window library).
// The parent is subclassed so the webview stays pinned to its section when the
// parent is resized (move the section with webview_set_bounds). The parent
// must already be created via window::window_create.
//
// LIMITATION: one webview per parent window — the embed association uses a
// single window property name shared module-wide, so attaching a second
// webview to the same window would overwrite the first's property and break
// its resize pinning.
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
    wv.attached = true
    wv.parent = parent
    wv.bounds = RECT { left : x, top : y, right : x + width, bottom : y + height }
    var err = wv_init(wv, 0)
    if(err != 0) {
        // Clean up partial state (unsubclasses the parent, destroys the widget).
        webview_destroy(wv)
        var msg = string("webview attach failed with code ")
        msg.append_integer(err as bigint)
        return std.Result.Err(WebViewError.InitFailed(msg))
    }
    wv.initialized = true
    return std.Result.Ok(std::Unit{})
}

// Move/resize the webview section inside the attached parent window (in the
// parent's client coordinates). No-op in standalone mode.
public func webview_set_bounds(wv : *mut WebView, x : int, y : int, width : int, height : int) {
    if(!wv.attached) {
        return
    }
    wv.bounds = RECT { left : x, top : y, right : x + width, bottom : y + height }
    if(wv.widget != null) {
        MoveWindow(wv.widget, x, y, width, height, 1)
        wv_resize_webview(wv)
    }
}

// Returns 0 on success, negative error code otherwise.
// NOTE: `wv` is expected to be created via WebView.make(), so all fields are
// already initialized (a memset here would wipe the title/width/height fields
// set by make()).
func wv_init(wv : *mut WebView, debug : int) : int {
    // Verify the WebView2 runtime is available (via the loader).
    var version_info : LPWSTR = null
    var hr = loader_get_browser_version(&raw mut wv.loader_lib, null, &raw mut version_info)
    var ok : int = 0
    if(SUCCEEDED(hr) && version_info != null) {
        ok = 1
    }
    if(version_info != null) {
        CoTaskMemFree(version_info as *mut void)
    }
    if(ok == 0) {
        return -1
    }

    var hInstance = GetModuleHandleW(null)

    CoInitializeEx(null, COINIT_APARTMENTTHREADED)
    if(SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
        // ok
    } else if(GetLastError() != ERROR_ACCESS_DENIED) {
        // non-fatal
    }

    var icon = LoadImageW(hInstance, IDI_APPLICATION, IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR) as HICON

    // Widget window that WebView2 is embedded into (shared by both modes).
    var widget_wc = zeroed<WNDCLASSEXW>()
    widget_wc.cbSize = sizeof(WNDCLASSEXW) as UINT
    widget_wc.hInstance = hInstance
    var widget_class_name : [64]ushort
    widen_to_buf("webview_widget", &raw mut widget_class_name[0], 64)
    widget_wc.lpszClassName = &raw widget_class_name[0]
    widget_wc.lpfnWndProc = wv_widget_proc as window::WNDPROC
    RegisterClassExW(&raw mut widget_wc)

    if(wv.attached) {
        // Embed mode: the widget is a child of the app-owned parent window,
        // occupying the section wv.bounds in the parent's client coordinates.
        // The parent is subclassed so the section tracks parent resizes.
        var parent_hwnd = wv.parent.hwnd
        wv.widget = CreateWindowExW(
            WS_EX_CONTROLPARENT,
            &raw widget_class_name[0],
            null,
            WS_CHILD,
            wv.bounds.left,
            wv.bounds.top,
            wv.bounds.right - wv.bounds.left,
            wv.bounds.bottom - wv.bounds.top,
            parent_hwnd,
            null,
            hInstance,
            wv as *mut void
        )
        if(wv.widget == null) {
            return -3
        }
        wv_attach_subclass(wv)
    } else {
        // Standalone mode: the top-level window is created through the window
        // library (lang/libs/window), and the widget fills its client area.
        // The app can mix native UI with the webview through wv.win (it owns
        // the window, its title/size/callbacks, etc.). Sizes are logical
        // (96-dpi units); the window library scales them by DPI.
        wv.win.width = wv.width
        wv.win.height = wv.height
        window::window_set_title(&raw mut wv.win, wv.title.data())
        var wres = window::window_create(&raw mut wv.win)
        if(wres is std::Result.Err) {
            return -2
        }
        window::window_set_user_data(&raw mut wv.win, wv as *mut void)
        window::window_set_resize_callback(&raw mut wv.win, wv_win_resize_cb)
        window::window_set_focus_callback(&raw mut wv.win, wv_win_focus_cb)

        wv.widget = CreateWindowExW(
            WS_EX_CONTROLPARENT,
            &raw widget_class_name[0],
            null,
            WS_CHILD,
            0,
            0,
            0,
            0,
            wv.win.hwnd,
            null,
            hInstance,
            wv as *mut void
        )
        if(wv.widget == null) {
            return -3
        }
        // size the widget to the window's current client area
        var rc : RECT
        if(GetClientRect(wv.win.hwnd, &raw mut rc)) {
            MoveWindow(wv.widget, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, 1)
        }
    }

    return wv_embed(wv, debug)
}

public func webview_run(wv : *mut WebView) {
    window::window_run()
}

public func webview_stop(wv : *mut WebView) {
    window::window_quit()
}

public func webview_title(wv : *mut WebView) : string {
    return wv.title
}

public func webview_set_title(wv : *mut WebView, title : *char) {
    wv.title = string("")
    wv.title.append_char_ptr(title)
    if(!wv.attached && window::window_is_created(&raw mut wv.win)) {
        window::window_set_title(&raw mut wv.win, title)
    }
}

func wv_set_size(wv : *mut WebView, width : int, height : int, hints : int) {
    if(wv.attached) {
        // embed mode: the section size is managed via webview_set_bounds
        return
    }
    if(!window::window_is_created(&raw mut wv.win)) {
        wv.win.width = width
        wv.win.height = height
        return
    }
    if(hints == WV_HINT_MAX) {
        window::window_set_max_size(&raw mut wv.win, width, height)
    } else if(hints == WV_HINT_MIN) {
        window::window_set_min_size(&raw mut wv.win, width, height)
    } else if(hints == WV_HINT_FIXED) {
        window::window_set_min_size(&raw mut wv.win, width, height)
        window::window_set_max_size(&raw mut wv.win, width, height)
    } else {
        window::window_set_size(&raw mut wv.win, width, height)
    }
}

public func webview_set_size(wv : *mut WebView, width : int, height : int) {
    wv.width = width
    wv.height = height
    wv_set_size(wv, width, height, WV_HINT_NONE)
}

public func webview_show(wv : *mut WebView) {
    if(wv.attached) {
        // embed mode: the app owns the parent window; just make sure the
        // webview section itself is visible
        if(wv.widget != null) {
            ShowWindow(wv.widget, SW_SHOW)
            SetFocus(wv.widget)
        }
        wv.visible = true
    } else if(window::window_is_created(&raw mut wv.win)) {
        window::window_show(&raw mut wv.win)
        window::window_focus(&raw mut wv.win)
        wv.visible = true
    }
}

public func webview_hide(wv : *mut WebView) {
    wv.visible = false
    if(wv.attached) {
        if(wv.widget != null) {
            ShowWindow(wv.widget, 0) // SW_HIDE — hide just the webview section
        }
    } else if(window::window_is_created(&raw mut wv.win)) {
        window::window_hide(&raw mut wv.win)
    }
}

public func webview_load_url(wv : *mut WebView, url : *char) {
    var wbuf : [4096]ushort
    widen_to_buf(url, &raw mut wbuf[0], 4096)
    if(wv.webview != null) {
        wv.webview.lpVtbl.Navigate(wv.webview, &raw wbuf[0])
    }
}

public func webview_evaluate_js(wv : *mut WebView, js : *char) {
    var wbuf : [32768]ushort
    widen_to_buf(js, &raw mut wbuf[0], 32768)
    if(wv.webview != null) {
        wv.webview.lpVtbl.ExecuteScript(wv.webview, &raw wbuf[0], null)
    }
}

// Evaluate JavaScript and receive the result through an asynchronous callback.
// `result` is a JSON-encoded string (WebView2's ExecuteScript result — e.g. a
// JS number arrives as "42", a JS string as "\"hello\"", an object as its
// JSON text) and is valid only during the callback call; copy it if needed.
// On evaluation failure `result` is null. The callback runs on the UI thread
// (inside the message loop), so it must not block.
public func webview_evaluate_js_result(
    wv : *mut WebView,
    js : *char,
    cb : JsResultCallback,
    user_data : *mut void
) {
    if(wv.webview == null) {
        return
    }
    var h = malloc(sizeof(JsEvalHandler)) as *mut JsEvalHandler
    if(h == null) {
        return
    }
    h.vtbl = &raw g_js_vtbl
    h.cb = cb
    h.user_data = user_data
    // ref_count starts at 0 (our implied reference). WebView2 AddRefs the
    // handler when ExecuteScript accepts it and Releases it after Invoke; the
    // Release that brings the count to 0 frees the struct (see js_release).
    h.ref_count = 0
    var wbuf : [32768]ushort
    widen_to_buf(js, &raw mut wbuf[0], 32768)
    var res = wv.webview.lpVtbl.ExecuteScript(
        wv.webview,
        &raw wbuf[0],
        &raw h.vtbl as *mut void
    )
    if(FAILED(res)) {
        // WebView2 never accepted the handler — drop our implied reference.
        js_release(&raw h.vtbl as *mut ICoreWebView2ExecuteScriptCompletedHandler)
    }
}

public func webview_load_html(wv : *mut WebView, html : *char) {
    var wbuf : [65536]ushort
    widen_to_buf(html, &raw mut wbuf[0], 65536)
    if(wv.webview != null) {
        wv.webview.lpVtbl.NavigateToString(wv.webview, &raw wbuf[0])
    }
}

public func webview_destroy(wv : *mut WebView) {
    wv.initialized = false
    wv.visible = false
    if(wv.handler != null) {
        env_release(&raw wv.handler.env_vtbl as *mut ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler)
        wv.handler = null
    }
    if(wv.webview != null) {
        wv.webview.lpVtbl.Release(wv.webview)
        wv.webview = null
    }
    if(wv.controller != null) {
        wv.controller.lpVtbl.Release(wv.controller)
        wv.controller = null
    }
    if(wv.widget != null) {
        DestroyWindow(wv.widget)
        wv.widget = null
    }
    if(wv.attached) {
        // Restore the parent's original window proc. WM_NCDESTROY normally
        // does this, but the app may destroy the webview before the window.
        // parent_wndproc != 0 means we actually subclassed the parent (wv_init
        // may have failed before wv_attach_subclass ran — never clobber the
        // parent's proc with 0 in that case).
        if(wv.parent != null && wv.parent_wndproc != 0) {
            wv_ensure_embed_prop_name()
            SetWindowLongPtrW(wv.parent.hwnd, GWLP_WNDPROC, wv.parent_wndproc)
            RemovePropW(wv.parent.hwnd, &raw g_embed_prop_name[0])
        }
        wv.attached = false
        wv.parent = null
        wv.parent_wndproc = 0
    } else if(window::window_is_created(&raw mut wv.win)) {
        window::window_destroy(&raw mut wv.win)
    }
    if(wv.loader_lib != null) {
        FreeLibrary(wv.loader_lib)
        wv.loader_lib = null
    }
    CoUninitialize()
}

// Access the webview's underlying window (from the window library). In
// standalone mode this is the top-level window the webview created; in embed
// mode it is the parent window the webview was attached to. Use it to mix
// native UI with the webview: add controls, handle callbacks, set title/size,
// etc. — all through the window library's API.
public func webview_window(wv : *mut WebView) : *mut window::Window {
    if(wv.attached && wv.parent != null) {
        return wv.parent
    }
    return &raw mut wv.win
}

// JS<->native binding is Linux-only (WebKitGTK script-dialog bridge). The
// Win32 WebView2 backend does not expose it yet.
public func webview_bind(wv : *mut WebView, handler : JsBindHandler) : std::Result<std::Unit, WebViewError> {
    return std.Result.Err(WebViewError.PlatformNotSupported())
}

} // end namespace webview

