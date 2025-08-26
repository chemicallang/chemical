// Copyright (c) Chemical Language Foundation 2025.

// NO platform headers are included. Everything is declared below.
// Windows x64 only (uses IMAGE_FILE_MACHINE_AMD64, x64 CONTEXT fields).

#include <stdarg.h>
#include <stdio.h>

// -------------------- basic CRT --------------------
typedef unsigned long ULONG;
typedef unsigned long long uintptr_t;
typedef unsigned long long size_t;
typedef unsigned long long uint64_t;
typedef unsigned long      uint32_t;
typedef long               int32_t;
typedef unsigned int       uint;
typedef int                bool;
#define true 1
#define false 0
#ifndef NULL
#define NULL 0
#endif

int printf(const char*, ...);
int snprintf(char*, size_t, const char*, ...);
void* memset(void*, int, size_t);
void* memcpy(void*, const void*, size_t);
void* malloc(size_t);
void  free(void*);

// signals (we avoid SIGSEGV—let SEH handle crashes)
int signal(int, void (*)(int));
int raise(int);
#define SIGINT   2
#define SIGILL   4
#define SIGFPE   8
#define SIGABRT 22
#define SIGTERM 15

// -------------------- calling conv -----------------
#if defined(_MSC_VER)
#  define WINAPI __stdcall
#  define CALLBACK __stdcall
#  define IMAGEAPI __stdcall
#else
#  define WINAPI __attribute__((stdcall))
#  define CALLBACK __attribute__((stdcall))
#  define IMAGEAPI __attribute__((stdcall))
#endif

// -------------------- Win32 base types -------------
typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef unsigned long       DWORD;
typedef long                LONG;
typedef unsigned long long  DWORD64;
typedef unsigned long long  ULONG64;
typedef unsigned long long  ULONG_PTR;
typedef void*               HANDLE;
typedef void*               PVOID;
typedef const char*         LPCSTR;
typedef char*               LPSTR;
typedef void*               LPVOID;
typedef unsigned long*      LPDWORD;
typedef unsigned short      USHORT;

// -------------------- Kernel32 / Ntdll -------------
__declspec(dllimport) HANDLE WINAPI GetCurrentProcess(void);
__declspec(dllimport) HANDLE WINAPI GetCurrentThread(void);
__declspec(dllimport) DWORD  WINAPI GetCurrentThreadId(void);
__declspec(dllimport) DWORD  WINAPI GetCurrentProcessId(void);
__declspec(dllimport) DWORD  WINAPI GetLastError(void);
__declspec(dllimport) void   WINAPI ExitProcess(unsigned int);
typedef LONG (WINAPI *LPTOP_LEVEL_EXCEPTION_FILTER)(struct _EXCEPTION_POINTERS*);
__declspec(dllimport) LPTOP_LEVEL_EXCEPTION_FILTER WINAPI SetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER);

typedef BOOL (WINAPI *PHANDLER_ROUTINE)(DWORD);
__declspec(dllimport) BOOL WINAPI SetConsoleCtrlHandler(PHANDLER_ROUTINE, BOOL);

// File APIs
__declspec(dllimport) HANDLE WINAPI CreateFileA(LPCSTR,DWORD,DWORD,LPVOID,DWORD,DWORD,HANDLE);
__declspec(dllimport) BOOL   WINAPI CloseHandle(HANDLE);

// ntdll unwind helpers we may use as fallback (not used in SEH path)
__declspec(dllimport) USHORT WINAPI RtlCaptureStackBackTrace(DWORD,DWORD,PVOID*,DWORD*);

// -------------------- SEH structs ------------------
typedef struct _EXCEPTION_RECORD {
    DWORD    ExceptionCode;
    DWORD    ExceptionFlags;
    struct _EXCEPTION_RECORD* ExceptionRecord;
    PVOID    ExceptionAddress;
    DWORD    NumberParameters;
    ULONG_PTR ExceptionInformation[15];
} EXCEPTION_RECORD, *PEXCEPTION_RECORD;

// x64 CONTEXT (subset with general regs; layout matches Windows SDK)
typedef struct _CONTEXT {
    // Home params
    DWORD64 P1Home, P2Home, P3Home, P4Home, P5Home, P6Home;
    DWORD   ContextFlags;
    DWORD   MxCsr;
    WORD    SegCs, SegDs, SegEs, SegFs, SegGs, SegSs;
    DWORD   EFlags;
    DWORD64 Dr0,Dr1,Dr2,Dr3,Dr6,Dr7;
    DWORD64 Rax,Rcx,Rdx,Rbx,Rsp,Rbp,Rsi,Rdi;
    DWORD64 R8,R9,R10,R11,R12,R13,R14,R15;
    DWORD64 Rip;
    // We omit floating state etc — not needed for StackWalk64 init.
} CONTEXT, *PCONTEXT;

typedef struct _EXCEPTION_POINTERS {
    PEXCEPTION_RECORD ExceptionRecord;
    PCONTEXT          ContextRecord;
} EXCEPTION_POINTERS, *PEXCEPTION_POINTERS;

// -------------------- DbgHelp (decls only) --------
#define IMAGE_FILE_MACHINE_I386   0x014c
#define IMAGE_FILE_MACHINE_AMD64  0x8664

#define SYMOPT_CASE_INSENSITIVE   0x00000001
#define SYMOPT_UNDNAME            0x00000002
#define SYMOPT_DEFERRED_LOADS     0x00000004
#define SYMOPT_LOAD_LINES         0x00000010

typedef enum _ADDRESS_MODE { AddrMode1616, AddrMode1632, AddrModeReal, AddrModeFlat } ADDRESS_MODE;

typedef struct _ADDRESS64 {
    DWORD64      Offset;
    WORD         Segment;
    ADDRESS_MODE Mode;
} ADDRESS64;

typedef struct _KDHELP64 {
    DWORD64 Thread;
    DWORD64 ThCallbackStack;
    DWORD64 ThCallbackBStore;
    DWORD64 NextCallback;
    DWORD64 FramePointer;
    DWORD64 KiCallUserMode;
    DWORD64 KeUserCallbackDispatcher;
    DWORD64 SystemRangeStart;
    DWORD64 KiUserExceptionDispatcher;
    DWORD64 UnusedAlignment;
    DWORD64 Reserved[4];
} KDHELP64;

typedef struct _STACKFRAME64 {
    ADDRESS64 AddrPC;
    ADDRESS64 AddrReturn;
    ADDRESS64 AddrFrame;
    ADDRESS64 AddrStack;
    ADDRESS64 AddrBStore;
    PVOID     FuncTableEntry;
    DWORD64   Params[4];
    BOOL      Far;
    BOOL      Virtual;
    DWORD64   Reserved[3];
    KDHELP64  KdHelp;
} STACKFRAME64, *LPSTACKFRAME64;

#define MAX_SYM_NAME 1024
typedef struct _SYMBOL_INFO {
    ULONG   SizeOfStruct;
    ULONG   TypeIndex;
    ULONG64 Reserved[2];
    ULONG64 Index;
    ULONG64 Size;
    ULONG64 ModBase;
    ULONG   Flags;
    ULONG64 Value;
    ULONG64 Address;
    ULONG   Register;
    ULONG   Scope;
    ULONG   Tag;
    ULONG   NameLen;
    ULONG   MaxNameLen;
    char    Name[1];
} SYMBOL_INFO, *PSYMBOL_INFO;

typedef struct _IMAGEHLP_LINE64 {
    DWORD  SizeOfStruct;
    PVOID  Key;
    DWORD  LineNumber;
    LPSTR  FileName;
    DWORD64 Address;
} IMAGEHLP_LINE64, *PIMAGEHLP_LINE64;

typedef BOOL (CALLBACK *PREAD_PROCESS_MEMORY_ROUTINE64)(
        HANDLE, DWORD64, PVOID, DWORD, LPDWORD);

typedef PVOID   (CALLBACK *PFUNCTION_TABLE_ACCESS_ROUTINE64)(HANDLE, DWORD64);
typedef DWORD64 (CALLBACK *PGET_MODULE_BASE_ROUTINE64)(HANDLE, DWORD64);
typedef DWORD64 (CALLBACK *PTRANSLATE_ADDRESS_ROUTINE64)(HANDLE, HANDLE, DWORD64);

__declspec(dllimport) BOOL   IMAGEAPI SymInitialize(HANDLE, LPCSTR, BOOL);
__declspec(dllimport) BOOL   IMAGEAPI SymCleanup(HANDLE);
__declspec(dllimport) DWORD  IMAGEAPI SymSetOptions(DWORD);
__declspec(dllimport) BOOL   IMAGEAPI SymFromAddr(HANDLE, DWORD64, DWORD64*, PSYMBOL_INFO);
__declspec(dllimport) BOOL   IMAGEAPI SymGetLineFromAddr64(HANDLE, DWORD64, DWORD*, PIMAGEHLP_LINE64);
__declspec(dllimport) PVOID  IMAGEAPI SymFunctionTableAccess64(HANDLE, DWORD64);
__declspec(dllimport) DWORD64 IMAGEAPI SymGetModuleBase64(HANDLE, DWORD64);
__declspec(dllimport) BOOL   IMAGEAPI StackWalk64(
        DWORD, HANDLE, HANDLE, LPSTACKFRAME64, PVOID,
        PREAD_PROCESS_MEMORY_ROUTINE64,
        PFUNCTION_TABLE_ACCESS_ROUTINE64,
        PGET_MODULE_BASE_ROUTINE64,
        PTRANSLATE_ADDRESS_ROUTINE64);

// Minidump (optional)
typedef struct _MINIDUMP_EXCEPTION_INFORMATION {
    DWORD ThreadId;
    PEXCEPTION_POINTERS ExceptionPointers;
    BOOL  ClientPointers;
} MINIDUMP_EXCEPTION_INFORMATION, *PMINIDUMP_EXCEPTION_INFORMATION;

typedef enum _MINIDUMP_TYPE {
    MiniDumpNormal = 0x00000000,
    MiniDumpWithDataSegs = 0x00000001,
    MiniDumpWithFullMemory = 0x00000002,
    MiniDumpWithHandleData = 0x00000004,
    MiniDumpWithThreadInfo = 0x00001000,
} MINIDUMP_TYPE;

__declspec(dllimport) BOOL WINAPI MiniDumpWriteDump(
        HANDLE, DWORD, HANDLE, MINIDUMP_TYPE,
        PMINIDUMP_EXCEPTION_INFORMATION, void*, void*);

// -------------------- constants & helpers ---------
#define GENERIC_WRITE   0x40000000
#define FILE_SHARE_WRITE 0x00000002
#define CREATE_ALWAYS   2

static void logf(const char* fmt, ...) {
    char buf[2048];
    va_list ap;
    va_start(ap, fmt);
    _vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
    buf[sizeof(buf) - 1] = '\0';
    va_end(ap);
    fputs(buf, stderr);
}

// -------------------- symbols init ----------------
static int g_sym_ready = 0;

static void init_symbols_once(void) {
    if (g_sym_ready) return;
    HANDLE proc = GetCurrentProcess();

    // Load lines + demangle + defer loads
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);

    if (!SymInitialize(proc, NULL, 1 /* invade process: load current modules */)) {
        logf("SymInitialize failed (err=%lu)\n", (unsigned long)GetLastError());
        return;
    }
    g_sym_ready = 1;
}

// -------------------- stack walker --------------
static void resolve_and_print(DWORD64 addr, unsigned idx) {
    HANDLE proc = GetCurrentProcess();
    char    namebuf[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
    PSYMBOL_INFO si = (PSYMBOL_INFO)namebuf;
    memset(si, 0, sizeof(namebuf));
    si->SizeOfStruct = sizeof(SYMBOL_INFO);
    si->MaxNameLen = MAX_SYM_NAME;

    DWORD64 disp = 0;
    if (SymFromAddr(proc, addr, &disp, si)) {
        logf("%02u: %s + 0x%llx", idx, si->Name, (unsigned long long)disp);
    } else {
        logf("%02u: [0x%llx]", idx, (unsigned long long)addr);
    }

    IMAGEHLP_LINE64 line;
    memset(&line, 0, sizeof(line));
    line.SizeOfStruct = sizeof(line);
    DWORD disp32 = 0;
    if (SymGetLineFromAddr64(proc, addr, &disp32, &line) && line.FileName) {
        logf("  at %s:%lu\n", line.FileName, (unsigned long)line.LineNumber);
    } else {
        logf("\n");
    }
}

static void walk_stack_from_context(PCONTEXT ctx) {
    init_symbols_once();

    STACKFRAME64 sf;
    memset(&sf, 0, sizeof(sf));
    sf.AddrPC.Offset    = ctx->Rip;
    sf.AddrPC.Mode      = AddrModeFlat;
    sf.AddrFrame.Offset = ctx->Rbp;
    sf.AddrFrame.Mode   = AddrModeFlat;
    sf.AddrStack.Offset = ctx->Rsp;
    sf.AddrStack.Mode   = AddrModeFlat;

    HANDLE proc = GetCurrentProcess();
    HANDLE th   = GetCurrentThread();

    unsigned i = 0;
    while (StackWalk64(IMAGE_FILE_MACHINE_AMD64,
                       proc, th, &sf, (PVOID)ctx,
                       (PREAD_PROCESS_MEMORY_ROUTINE64)NULL,
                       SymFunctionTableAccess64,
                       SymGetModuleBase64,
                       NULL)) {
        if (sf.AddrPC.Offset == 0) break;
        resolve_and_print(sf.AddrPC.Offset, i++);
        if (i > 128) break; // sanity
    }
    if (i == 0) {
        // Fallback: capture current stack if walk failed
        PVOID addrs[64];
        USHORT n = RtlCaptureStackBackTrace(0, 64, addrs, NULL);
        for (USHORT j = 0; j < n; ++j) resolve_and_print((DWORD64)(ULONG_PTR)addrs[j], j);
    }
}

// -------------------- minidump (optional) --------
static void write_minidump(PEXCEPTION_POINTERS ep) {
    HANDLE h = CreateFileA("crash.dmp", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, NULL);
    if (h == NULL || h == (HANDLE)(uintptr_t)-1) {
        logf("MiniDump: failed to create crash.dmp\n");
        return;
    }
    MINIDUMP_EXCEPTION_INFORMATION mei;
    mei.ThreadId = GetCurrentThreadId();
    mei.ExceptionPointers = ep;
    mei.ClientPointers = 0;
    if (MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), h, MiniDumpWithThreadInfo, &mei, NULL, NULL)) {
        logf("MiniDump: wrote crash.dmp\n");
    } else {
        logf("MiniDump: failed (err=%lu)\n", (unsigned long)GetLastError());
    }
    CloseHandle(h);
}

// -------------------- handlers -------------------
static LONG WINAPI seh_filter(struct _EXCEPTION_POINTERS *ep) {
    logf("Unhandled exception 0x%08lX at %p\n",
         (unsigned long)ep->ExceptionRecord->ExceptionCode,
         ep->ExceptionRecord->ExceptionAddress);

    write_minidump(ep);
    walk_stack_from_context(ep->ContextRecord);

    // Terminate quickly; don't return to an unsafe state.
    ExitProcess(1);
    return 1;
}

static BOOL WINAPI console_handler(DWORD ctrl) {
    logf("Console event %lu\n", (unsigned long)ctrl);
    // Light, non-fatal stack for diagnostics
    PVOID addrs[32];
    USHORT n = RtlCaptureStackBackTrace(0, 32, addrs, NULL);
    for (USHORT i=0;i<n;++i) resolve_and_print((DWORD64)(ULONG_PTR)addrs[i], i);
    return 0; // let default handling continue
}

// -------------------- public API -----------------
#ifdef __cplusplus
extern "C" {
#endif
void install_stacktrace_handler(void) {
    SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER) seh_filter);
    SetConsoleCtrlHandler(console_handler, 1);

    // Keep only non-crash signals here; let SEH catch crashes for CONTEXT
    // signal(SIGABRT, [](int){ logf("SIGABRT\n"); });
    // signal(SIGFPE,  [](int){ logf("SIGFPE\n"); });
    // signal(SIGILL,  [](int){ logf("SIGILL\n"); });
    // signal(SIGINT,  [](int){ logf("SIGINT\n"); });
    // signal(SIGTERM, [](int){ logf("SIGTERM\n"); });

    init_symbols_once();
}
#ifdef __cplusplus
}
#endif

void sub_crasher() {
    *(volatile int*)0 = 42; // AV
}

void crasher() {
    sub_crasher();
}

// -------------------- test main ------------------
int main(void) {
    install_stacktrace_handler();
    logf("Installed. Crashing in 1s...\n");
    for (volatile int i=0;i<200000000;i++) {}
    crasher();
    return 0;
}