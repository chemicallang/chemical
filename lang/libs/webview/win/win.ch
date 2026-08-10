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
public type LPCREATESTRUCTW = *mut CREATESTRUCTW
public type LPMINMAXINFO = *mut MINMAXINFO
public type LPRECT = *mut RECT

// ===========================================================================
// Win32 structs
// ===========================================================================

public struct GUID {
    var Data1 : DWORD
    var Data2 : WORD
    var Data3 : WORD
    var Data4 : [8]BYTE
}

public type IID = GUID

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
@extern @stdcall @dllimport public func GetModuleFileNameW(lpFilename : LPWSTR, nSize : DWORD) : DWORD
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
    // --- WebView2 backend state (pure-C port) ---
    var window : HWND
    var widget : HWND
    var webview : *mut ICoreWebView2
    var controller : *mut ICoreWebView2Controller
    var handler : *mut Webview2ComHandler
    var loader_lib : HMODULE
    var minsz : POINT
    var maxsz : POINT
    var dpi : int
    var window_shown : int

    // --- cross-platform API fields (must match posix/linux.ch) ---
    var title : string
    var width : int
    var height : int
    var visible : bool
    var initialized : bool

    @make
    func make() : WebView {
        return WebView {
            window : null,
            widget : null,
            webview : null,
            controller : null,
            handler : null,
            loader_lib : null,
            minsz : POINT { x : 0, y : 0 },
            maxsz : POINT { x : 0, y : 0 },
            dpi : 0,
            window_shown : 0,
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

func wv_window_proc(hwnd : HWND, msg : UINT, wp : WPARAM, lp : LPARAM) : LRESULT {
    var wv = GetWindowLongPtrW(hwnd, GWLP_USERDATA) as *mut WebView
    if(msg == WM_NCCREATE) {
        var cs = lp as *mut CREATESTRUCTW
        wv = cs.lpCreateParams as *mut WebView
        wv.window = hwnd
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, wv as LONG_PTR)
        EnableNonClientDpiScaling(hwnd)
        return 1
    }
    if(wv == null) {
        return DefWindowProcW(hwnd, msg, wp, lp)
    }
    switch(msg) {
        WM_SIZE => {
            if(wv.widget != null) {
                var rc : RECT
                if(GetClientRect(hwnd, &raw mut rc)) {
                    MoveWindow(wv.widget, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, 1)
                }
            }
        }
        WM_CLOSE => {
            DestroyWindow(hwnd)
        }
        WM_DESTROY => {
            wv.window = null
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0)
            PostQuitMessage(0)
        }
        WM_GETMINMAXINFO => {
            var lpmmi = lp as *mut MINMAXINFO
            if(wv.maxsz.x > 0 && wv.maxsz.y > 0) {
                lpmmi.ptMaxSize = wv.maxsz
                lpmmi.ptMaxTrackSize = wv.maxsz
            }
            if(wv.minsz.x > 0 && wv.minsz.y > 0) {
                lpmmi.ptMinTrackSize = wv.minsz
            }
        }
        WM_ACTIVATE => {
            var wplow = (wp as DWORD) & (0xFFFF as DWORD)
            if(wplow != 0) { // LOWORD(wp) != WA_INACTIVE
                if(wv.controller != null) {
                    wv.controller.lpVtbl.MoveFocus(wv.controller, MOVE_FOCUS_REASON_PROGRAMMATIC)
                }
            }
        }
        default => {
            return DefWindowProcW(hwnd, msg, wp, lp)
        }
    }
    return 0
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
    GetModuleFileNameW(&raw mut currentExePath[0], 260)
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

    // Top-level window.
    var wc = zeroed<WNDCLASSEXW>()
    wc.cbSize = sizeof(WNDCLASSEXW) as UINT
    wc.hInstance = hInstance
    var wc_class_name : [64]ushort
    widen_to_buf("webview", &raw mut wc_class_name[0], 64)
    wc.lpszClassName = &raw wc_class_name[0]
    wc.hIcon = icon
    wc.lpfnWndProc = wv_window_proc as WNDPROC
    RegisterClassExW(&raw mut wc)

    var empty_wide : [2]ushort
    empty_wide[0] = 0
    // NOTE: the SDK header defines CreateWindowW as a macro expanding to
    // CreateWindowExW(0, ...); CreateWindowExW is the real user32 export.
    // NOTE: the window is created at wv.width x wv.height (the make() defaults,
    // or whatever was set before create) so that create -> show -> run works
    // without a webview_set_size call, matching posix/linux.ch which applies
    // wv.width/wv.height at creation time.
    wv.window = CreateWindowExW(
        0,
        &raw wc_class_name[0],
        &raw empty_wide[0],
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        wv.width,
        wv.height,
        null,
        null,
        hInstance,
        wv as *mut void
    )
    if(wv.window == null) {
        return -2
    }
    wv.dpi = GetDpiForWindow(wv.window) as int

    // Match posix/linux.ch: a title set via make() (or a struct literal) is
    // applied to the window at creation time too.
    if(wv.title.size() > 0) {
        var title_buf : [512]ushort
        widen_to_buf(wv.title.data(), &raw mut title_buf[0], 512)
        SetWindowTextW(wv.window, &raw title_buf[0])
    }

    // Widget window that WebView2 is embedded into.
    var widget_wc = zeroed<WNDCLASSEXW>()
    widget_wc.cbSize = sizeof(WNDCLASSEXW) as UINT
    widget_wc.hInstance = hInstance
    var widget_class_name : [64]ushort
    widen_to_buf("webview_widget", &raw mut widget_class_name[0], 64)
    widget_wc.lpszClassName = &raw widget_class_name[0]
    widget_wc.lpfnWndProc = wv_widget_proc as WNDPROC
    RegisterClassExW(&raw mut widget_wc)

    wv.widget = CreateWindowExW(
        WS_EX_CONTROLPARENT,
        &raw widget_class_name[0],
        null,
        WS_CHILD,
        0,
        0,
        0,
        0,
        wv.window,
        null,
        hInstance,
        wv as *mut void
    )
    if(wv.widget == null) {
        return -3
    }

    return wv_embed(wv, debug)
}

public func webview_run(wv : *mut WebView) {
    var msg : MSG
    while(GetMessageW(&raw mut msg, null, 0, 0) > 0) {
        TranslateMessage(&raw mut msg)
        DispatchMessageW(&raw mut msg)
    }
}

public func webview_stop(wv : *mut WebView) {
    PostQuitMessage(0)
}

public func webview_title(wv : *mut WebView) : string {
    return wv.title
}

public func webview_set_title(wv : *mut WebView, title : *char) {
    wv.title = string("")
    wv.title.append_char_ptr(title)
    var wbuf : [512]ushort
    widen_to_buf(title, &raw mut wbuf[0], 512)
    SetWindowTextW(wv.window, &raw wbuf[0])
}

func wv_set_size(wv : *mut WebView, width : int, height : int, hints : int) {
    var style = GetWindowLongPtrW(wv.window, GWL_STYLE)
    if(hints == WV_HINT_FIXED) {
        style = style & ~((WS_THICKFRAME | WS_MAXIMIZEBOX) as LONG_PTR)
    } else {
        style = style | ((WS_THICKFRAME | WS_MAXIMIZEBOX) as LONG_PTR)
    }
    SetWindowLongPtrW(wv.window, GWL_STYLE, style)

    if(hints == WV_HINT_MAX) {
        wv.maxsz.x = width
        wv.maxsz.y = height
    } else if(hints == WV_HINT_MIN) {
        wv.minsz.x = width
        wv.minsz.y = height
    } else {
        var dpi = GetDpiForWindow(wv.window) as int
        wv.dpi = dpi
        var scaled_w = (width * dpi) / 96
        var scaled_h = (height * dpi) / 96
        var r = RECT { left : 0, top : 0, right : scaled_w, bottom : scaled_h }
        AdjustWindowRectExForDpi(&raw mut r, style as DWORD, 0, 0, dpi as UINT)
        SetWindowPos(
            wv.window,
            null,
            0,
            0,
            r.right - r.left,
            r.bottom - r.top,
            SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE | SWP_FRAMECHANGED
        )
    }
}

public func webview_set_size(wv : *mut WebView, width : int, height : int) {
    wv.width = width
    wv.height = height
    wv_set_size(wv, width, height, WV_HINT_NONE)
}

public func webview_show(wv : *mut WebView) {
    if(wv.window != null) {
        ShowWindow(wv.window, SW_SHOW)
        UpdateWindow(wv.window)
        SetFocus(wv.window)
        SetForegroundWindow(wv.window)
        wv.window_shown = 1
        wv.visible = true
    }
}

public func webview_hide(wv : *mut WebView) {
    wv.visible = false
    if(wv.window != null) {
        ShowWindow(wv.window, 0) // SW_HIDE
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
    if(wv.window != null) {
        DestroyWindow(wv.window)
        wv.window = null
    }
    if(wv.loader_lib != null) {
        FreeLibrary(wv.loader_lib)
        wv.loader_lib = null
    }
    CoUninitialize()
}

} // end namespace webview

