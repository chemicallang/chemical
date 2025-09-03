// type DWORD = ulong;
public type UINT = uint;
public type BOOL = int;
public type WORD = ushort;
public type DWORD64 = ubigint;
public type LONG = long;
public type HANDLE = *void;
// type PVOID = *void;
public type LPCSTR = *char;
public type LPSTR = *char;
public type LPVOID = *void;
public type ULONG = ulong;
public type PULONG = *ulong;
public type ULONG64 = ubigint;
public type ULONG_PTR = ubigint;
public type USHORT = ushort;
public type LPDWORD = *ulong;

// x64 CONTEXT (subset with general regs; layout matches Windows SDK)
public struct _CONTEXT {
    // Home params
    var P1Home : DWORD64
    var P2Home : DWORD64
    var P3Home : DWORD64
    var P4Home : DWORD64
    var P5Home : DWORD64
    var P6Home : DWORD64
    var ContextFlags : DWORD;
    var   MxCsr : DWORD;
    var    SegCs : WORD
    var SegDs : WORD
    var SegEs : WORD
    var SegFs : WORD
    var SegGs : WORD
    var SegSs : WORD
    var   EFlags : DWORD;
    var Dr0 : DWORD64
    var Dr1 : DWORD64
    var Dr2 : DWORD64
    var Dr3 : DWORD64
    var Dr6 : DWORD64
    var Dr7 : DWORD64;
    var Rax : DWORD64
    var Rcx : DWORD64
    var Rdx : DWORD64
    var Rbx : DWORD64
    var Rsp : DWORD64
    var Rbp : DWORD64
    var Rsi : DWORD64
    var Rdi : DWORD64;
    var R8 : DWORD64
    var R9 : DWORD64
    var R10 : DWORD64
    var R11 : DWORD64
    var R12 : DWORD64
    var R13 : DWORD64
    var R14 : DWORD64
    var R15 : DWORD64
    var Rip : DWORD64;
    // We omit floating state etc — not needed for StackWalk64 init.
};

public type CONTEXT = _CONTEXT;
public type PCONTEXT = *mut _CONTEXT

/* Basic kernel32/ntdll prototypes (manual declarations). VERIFY if needed. */
@extern @dllimport @stdcall public func GetCurrentProcess() : HANDLE;
@extern @dllimport @stdcall public func GetCurrentThread() : HANDLE;
@extern @dllimport @stdcall public func GetCurrentThreadId() : DWORD;
@extern @dllimport @stdcall public func GetCurrentProcessId() : DWORD;
@extern @dllimport @stdcall public func GetLastError() : DWORD;
@extern @dllimport @stdcall public func ExitProcess(uExitCode : UINT);

// TODO: @stdcall
public type LPTOP_LEVEL_EXCEPTION_FILTER = (ptr : *mut void) => LONG

@extern @dllimport @stdcall
public func SetUnhandledExceptionFilter(lpTopLevelExceptionFilter : LPTOP_LEVEL_EXCEPTION_FILTER) : LPTOP_LEVEL_EXCEPTION_FILTER;

// TODO: @stdcall
public type PHANDLER_ROUTINE = (dwCtrlType : DWORD) => BOOL

@extern @dllimport @stdcall public func SetConsoleCtrlHandler(HandlerRoutine : PHANDLER_ROUTINE, Add : BOOL) : BOOL
@extern @dllimport @stdcall public func RtlCaptureStackBackTrace(FramesToSkip : DWORD, FramesToCapture : DWORD, BackTrace : *mut PVOID, BackTraceHash : *mut DWORD) : USHORT

/* PSAPI functions for enumerating modules (manual) */
public struct _MODULEINFO {
    var lpBaseOfDll : *void;
    var SizeOfImage : DWORD;
    var EntryPoint : *mut void;
};

public type MODULEINFO = _MODULEINFO

@extern @dllimport @stdcall public func EnumProcessModules(hProcess : HANDLE, lphModule : *mut void, cb : DWORD, lpcbNeeded : LPDWORD) : BOOL
@extern @dllimport @stdcall public func GetModuleInformation(hProcess : HANDLE, hModule : *mut void, pmi : *mut MODULEINFO, cb : DWORD) : BOOL
@extern @dllimport @stdcall public func GetModuleFileNameExA(hProcess : HANDLE, hModule : *mut void, lpFilename : LPSTR, nSize : DWORD) : DWORD
@extern @dllimport @stdcall public func GetModuleBaseNameA(hProcess : HANDLE, hModule : *mut void, lpBaseName : LPSTR, nSize : DWORD) : DWORD

/* IMAGEHLP / DbgHelp types and functions (manual, x64-focused) */
comptime const IMAGE_FILE_MACHINE_I386 =   0x014c
comptime const IMAGE_FILE_MACHINE_AMD64 =  0x8664

comptime const SYMOPT_CASE_INSENSITIVE =   0x00000001
comptime const SYMOPT_UNDNAME =            0x00000002
comptime const SYMOPT_DEFERRED_LOADS =     0x00000004
comptime const SYMOPT_LOAD_LINES =         0x00000010

public enum _ADDRESS_MODE {
    AddrMode1616,
    AddrMode1632,
    AddrModeReal,
    AddrModeFlat
};

public type ADDRESS_MODE = _ADDRESS_MODE
public comptime const AddrMode1616 = _ADDRESS_MODE.AddrMode1616
public comptime const AddrMode1632 = _ADDRESS_MODE.AddrMode1632
public comptime const AddrModeReal = _ADDRESS_MODE.AddrModeReal
public comptime const AddrModeFlat = _ADDRESS_MODE.AddrModeFlat

public struct _ADDRESS64 {
    var Offset : DWORD64;
    var Segment : WORD;
    var Mode : ADDRESS_MODE;
};

public type ADDRESS64 = _ADDRESS64

public struct _KDHELP64 {
    var Thread : DWORD64; var ThCallbackStack : DWORD64; var ThCallbackBStore : DWORD64; var NextCallback : DWORD64; var FramePointer : DWORD64;
    var KiCallUserMode : DWORD64; var KeUserCallbackDispatcher : DWORD64; var SystemRangeStart : DWORD64; var KiUserExceptionDispatcher : DWORD64;
    var UnusedAlignment : DWORD64; var Reserved : [4]DWORD64;
};

public type KDHELP64 = _KDHELP64

public struct _STACKFRAME64 {
    var AddrPC : ADDRESS64; var AddrReturn : ADDRESS64; var AddrFrame : ADDRESS64; var AddrStack : ADDRESS64; var AddrBStore : ADDRESS64;
    var FuncTableEntry : PVOID; var Params : [4]DWORD64; var Far : BOOL; var Virtual : BOOL; var Reserved : [3]DWORD64; var KdHelp : KDHELP64;
};

public type STACKFRAME64 = _STACKFRAME64
public type LPSTACKFRAME64 = *mut _STACKFRAME64

public comptime const MAX_SYM_NAME = 1024

public struct _SYMBOL_INFO {
    var SizeOfStruct : ULONG;
    var TypeIndex : ULONG;
    var Reserved : [2]ULONG64;
    var Index : ULONG64;
    var Size : ULONG64;
    var ModBase : ULONG64;
    var Flags : ULONG;
    var Value : ULONG64;
    var Address : ULONG64;
    var Register : ULONG;
    var Scope : ULONG;
    var Tag : ULONG;
    var NameLen : ULONG;
    var MaxNameLen : ULONG;
    var Name : [1]char;
};

public type SYMBOL_INFO = _SYMBOL_INFO
public type PSYMBOL_INFO = *mut _SYMBOL_INFO

public struct _IMAGEHLP_LINE64 {
    var SizeOfStruct : DWORD;
    var Key : PVOID;
    var LineNumber : DWORD;
    var FileName : LPSTR;
    var Address : DWORD64;
};

public type IMAGEHLP_LINE64 = _IMAGEHLP_LINE64
public type PIMAGEHLP_LINE64 = *mut _IMAGEHLP_LINE64

/* DbgHelp prototypes (manual). You may want to dynamically load these in future. */
@extern @dllimport @stdcall public func SymInitialize(hProcess : HANDLE, UserSearchPath : LPCSTR, fInvadeProcess : BOOL) : BOOL
@extern @dllimport @stdcall public func SymCleanup(hProcess : HANDLE) : BOOL
@extern @dllimport @stdcall public func SymSetOptions(SymOptions : DWORD) : DWORD
@extern @dllimport @stdcall public func SymFromAddr(hProcess : HANDLE, Address : DWORD64, Displacement : *mut DWORD64, Symbol : PSYMBOL_INFO) : BOOL
@extern @dllimport @stdcall public func SymGetLineFromAddr64(hProcess : HANDLE, qwAddr : DWORD64, pdwDisplacement : *mut DWORD, Line : PIMAGEHLP_LINE64) : BOOL
@extern @dllimport @stdcall public func SymFunctionTableAccess64(hProcess : HANDLE, AddrBase : DWORD64) : PVOID
@extern @dllimport @stdcall public func SymGetModuleBase64(hProcess : HANDLE, Addr : DWORD64) : DWORD64
@extern @dllimport @stdcall public func SymGetSymFromAddr64(hProcess : HANDLE, qwAddr : DWORD64, pdwDisplacement : *mut DWORD64, Symbol : *mut void) : BOOL

//TODO: @stdcall
public type PREAD_PROCESS_MEMORY_ROUTINE64 = (ph : HANDLE, d : DWORD64, pv : PVOID, dw : DWORD, lpd : LPDWORD) => BOOL

//TODO: @stdcall
public type PFUNCTION_TABLE_ACCESS_ROUTINE64 = (ph : HANDLE, dw : DWORD64) => PVOID

//TODO: @stdcall
public type PGET_MODULE_BASE_ROUTINE64 = (ph : HANDLE, dw : DWORD64) => DWORD64;

//TODO: @stdcall
public type PTRANSLATE_ADDRESS_ROUTINE64 = (ph : HANDLE, ph2 : HANDLE, dw : DWORD64) => DWORD64

@extern @dllimport @stdcall
public func StackWalk64(
    MachineType : DWORD,
    hProcess : HANDLE,
    hThread : HANDLE,
    StackFrame : LPSTACKFRAME64,
    ContextRecord : PVOID,
    ReadMemoryRoutine : PREAD_PROCESS_MEMORY_ROUTINE64,
    FunctionTableAccessRoutine : PFUNCTION_TABLE_ACCESS_ROUTINE64,
    GetModuleBaseRoutine : PGET_MODULE_BASE_ROUTINE64,
    TranslateAddress : PTRANSLATE_ADDRESS_ROUTINE64
) : BOOL

/* Optional: SymLoadModule64 */
@extern @dllimport @stdcall
public func SymLoadModule64(hProcess : HANDLE, hFile : *mut void, ImageName : LPCSTR, ModuleName : LPCSTR, BaseOfDll : DWORD64, SizeOfDll : DWORD) : DWORD64

/* Optional helper to get NT headers; minimal declaration (VERIFY) */
public struct _IMAGE_NT_HEADERS {
    // Minimal: We only need FileHeader.Machine. Put small structs to reach that field.
    struct {
        var Machine : WORD;
        var NumberOfSections : WORD;
        var TimeDateStamp : DWORD;
    } FileHeader;
};

public type IMAGE_NT_HEADERS = _IMAGE_NT_HEADERS

@dllimport @stdcall public func ImageNtHeader(Base : *void) : *mut IMAGE_NT_HEADERS; /* VERIFY: some toolchains provide this */

/* Implementation: get module list and load symbol modules */
func load_process_modules_and_register_symbols(process : HANDLE) {
    var cbNeeded : DWORD = 0;
    /* Query modules twice like common pattern */
    EnumProcessModules(process, null, 0, &cbNeeded);
    if (cbNeeded == 0) return;
    /* allocate buffer for handles */
    var modules = malloc(cbNeeded) as *mut *mut void;
    if (!modules) return;
    if (!EnumProcessModules(process, modules, cbNeeded, &cbNeeded)) { free(modules); return; }
    var count = cbNeeded / sizeof(*mut void);
    var filename : [4096]char;
    var modulename : [512]char;
    for (var i : int = 0; i < count; ++i) {
        var mi : MODULEINFO;
        if (!GetModuleInformation(process, modules[i], &mi, sizeof(mi))) continue;
        /* get full path and base name */
        GetModuleFileNameExA(process, modules[i], filename, sizeof(filename));
        GetModuleBaseNameA(process, modules[i], modulename, sizeof(modulename));
        /* register module with dbghelp */
        SymLoadModule64(process, null, filename, modulename, (mi.lpBaseOfDll as uintptr_t) as DWORD64, mi.SizeOfImage);
    }
    free(modules);
}

/* Correctly prepare SYMBOL_INFO buffer */
// TODO: crash here when si is used with typealias
func prepare_symbol_info_buffer(buffer : *mut char, si : *mut _SYMBOL_INFO) {
    /* buffer must be at least sizeof(SYMBOL_INFO) + (MAX_SYM_NAME-1) */
    memset(buffer, 0, sizeof(SYMBOL_INFO) + MAX_SYM_NAME);
    si.MaxNameLen = MAX_SYM_NAME;
    /* set SizeOfStruct to include the Name flexible array space as required by SymFromAddr */
    si.SizeOfStruct = (sizeof(SYMBOL_INFO) + (MAX_SYM_NAME - 1) * sizeof(char)) as ULONG;
}

/* Resolve single address and print function + file:line (if available). We print pointer also */
func resolve_and_print_win(addr : DWORD64, idx : uint) {
    var proc : HANDLE = GetCurrentProcess();
    var buf_sym : char[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
    // TODO: crash here when using typealias
    var si : *mut _SYMBOL_INFO = buf_sym as *mut _SYMBOL_INFO;
    prepare_symbol_info_buffer(buf_sym, si);
    var disp : DWORD64 = 0;
    if (SymFromAddr(proc, addr, &disp, si)) {
        /* Print name + offset + pointer (pointer printing is optional; remove if undesired) */
        printf("%02u: %s + 0x%llx [0x%llx]", idx, si.Name, disp as ubigint, addr as ubigint);
    } else {
        var err : DWORD = GetLastError() as DWORD;
        var modBase : DWORD64 = SymGetModuleBase64(proc, addr);
        printf("%02u: [0x%llx] (SymFromAddr failed err=%lu, module_base=0x%llx)", idx, addr as ubigint, err as ulong, modBase as ubigint);
    }

    var line : IMAGEHLP_LINE64;
    memset(&line, 0, sizeof(line));
    line.SizeOfStruct = sizeof(line);
    var disp32 : DWORD = 0;
    if (SymGetLineFromAddr64(proc, addr, &disp32, &line) && line.FileName) {
        printf("  at %s:%lu\n", line.FileName, line.LineNumber as ulong);
    } else {
        printf("\n");
    }
}

struct CrashHandlerParam {
   var ExceptionRecord : PVOID;
   var ContextRecord : PVOID;
}

/* The main Windows exception handler function (to be used in __except) */
@stdcall
func CrashHandlerException(ep_void : *mut void) : LONG {
    /* ep_void is EXCEPTION_POINTERS* */
    var process : HANDLE = GetCurrentProcess();
    var thread : HANDLE = GetCurrentThread();
    /* Try to avoid installing symbols if debugger present */
    /* You can decide to return EXCEPTION_CONTINUE_SEARCH if IsDebuggerPresent() (not declared here) */

    // TODO:
    // printf("%s: Program crashed\n", __FUNCTION__);

    /* Initialize dbghelp */
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
    if (!SymInitialize(process, null, 0)) {
        printf("SymInitialize failed (err=%lu)\n", GetLastError() as ulong);
    } else {
        /* load modules */
        load_process_modules_and_register_symbols(process);
    }

    /* Extract context and do StackWalk64 */
    /* ep_void is actually EXCEPTION_POINTERS*, but we don't declare full struct; just access as needed */
    var ep = ep_void as *mut CrashHandlerParam;
    var ctx = ep.ContextRecord as *mut CONTEXT;

    var frame : STACKFRAME64;
    memset(&frame, 0, sizeof(frame));
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Mode = AddrModeFlat;

if(def.x86_64) {
    frame.AddrPC.Offset = ctx.Rip;
    frame.AddrStack.Offset = ctx.Rsp;
    frame.AddrFrame.Offset = ctx.Rbp;
    var imageType : DWORD = IMAGE_FILE_MACHINE_AMD64;
} else {
    /* 32-bit minimal support (VERIFY) */
    frame.AddrPC.Offset = 0 as DWORD64; /* placeholder */
    frame.AddrStack.Offset = 0 as DWORD64;
    frame.AddrFrame.Offset = 0 as DWORD64;
    var imageType : DWORD = IMAGE_FILE_MACHINE_I386;
}
    printf("Dumping backtrace:\n");
    var i : uint = 0;
    while (StackWalk64(imageType, process, thread, &frame, ctx, null, SymFunctionTableAccess64, SymGetModuleBase64, null)) {
        if (frame.AddrPC.Offset == 0) break;
        resolve_and_print_win(frame.AddrPC.Offset, i++);
        if (i > 256) break;
    }

    SymCleanup(process);
    /* Let OS continue default handling */
    return 1; /* EXCEPTION_CONTINUE_SEARCH would be 0; adjust if you want */
}

/* Windows export to install handler via SetUnhandledExceptionFilter */
public func install_crash_handler(exe_path : *char, onCrash : () => void) {
    /* Install unhandled exception filter to our CrashHandlerException wrapper */
    SetUnhandledExceptionFilter(CrashHandlerException as LPTOP_LEVEL_EXCEPTION_FILTER);
    /* Optionally set console handler etc. (omitted) */
}