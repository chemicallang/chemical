/* Pure C crash handler.
 * - No C++ used.
 * - Windows: manual declarations for Win32 & DbgHelp APIs (no windows.h)
 * - Linux: uses backtrace/backtrace_symbols + addr2line via popen
 *
 * Build/test notes:
 *  - Windows: compile with PDB generation (CodeView) so DbgHelp can resolve symbols.
 *    Example (clang-cl): clang-cl /Zi /EHsc crash_handler_c.c dbghelp.lib psapi.lib
 *    Or (msvc): cl /Zi crash_handler_c.c dbghelp.lib psapi.lib
 *  - Linux: build with -rdynamic so backtrace_symbols can show function names:
 *    gcc -g -rdynamic crash_handler_c.c -o crash_handler_c
 *
 * You can wire this into your code:
 *  - On Windows: use the provided CrashHandlerException(EXCEPTION_POINTERS* ep) in an __except
 *    (or call install function to set SetUnhandledExceptionFilter to handler version).
 *  - On Linux: call crash_handler_install() early in process start.
 *
 * IMPORTANT: I declared many Windows types & prototypes manually. For x64 these match common MSDN
 * signatures but verify if you change architecture or compiler.
 */

/* Standard C headers allowed */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>

/* ---------- Platform selection ---------- */
#if defined(_WIN32) || defined(_WIN64)
#define PLATFORM_WINDOWS 1
#else
#define PLATFORM_LINUX 1
#endif

/* -------------------- Shared helpers -------------------- */
static void logf(const char *fmt, ...) {
    char buf[2048];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    fputs(buf, stderr);
    fflush(stderr);
}

/* -------------------- WINDOWS IMPLEMENTATION -------------------- */
#if defined(PLATFORM_WINDOWS)

typedef unsigned long       DWORD;
typedef unsigned int        UINT;
typedef int                 BOOL;
typedef unsigned short      WORD;
typedef unsigned long long  DWORD64;
typedef long                LONG;
typedef void*               HANDLE;
typedef void*               PVOID;
typedef const char*         LPCSTR;
typedef char*               LPSTR;
typedef void*               LPVOID;
typedef unsigned long       ULONG;
typedef unsigned long*      PULONG;
typedef unsigned long long  ULONG64;
typedef unsigned long long  ULONG_PTR;
typedef unsigned short      USHORT;
typedef unsigned long*      LPDWORD;
#ifndef NULL
#define NULL 0
#endif

/* calling convention macro (msvc / other) */
#if defined(_MSC_VER)
  #define WINAPI __stdcall
  #define CALLBACK __stdcall
  #define IMAGEAPI __stdcall
#else
  #define WINAPI __attribute__((stdcall))
  #define CALLBACK __attribute__((stdcall))
  #define IMAGEAPI __attribute__((stdcall))
#endif

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

/* Basic kernel32/ntdll prototypes (manual declarations). VERIFY if needed. */
__declspec(dllimport) HANDLE WINAPI GetCurrentProcess(void);
__declspec(dllimport) HANDLE WINAPI GetCurrentThread(void);
__declspec(dllimport) DWORD WINAPI GetCurrentThreadId(void);
__declspec(dllimport) DWORD WINAPI GetCurrentProcessId(void);
__declspec(dllimport) DWORD WINAPI GetLastError(void);
__declspec(dllimport) void   WINAPI ExitProcess(UINT uExitCode);
typedef LONG (WINAPI *LPTOP_LEVEL_EXCEPTION_FILTER)(void* /*EXCEPTION_POINTERS* */);
__declspec(dllimport) LPTOP_LEVEL_EXCEPTION_FILTER WINAPI SetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter);
typedef BOOL (WINAPI *PHANDLER_ROUTINE)(DWORD dwCtrlType);
__declspec(dllimport) BOOL WINAPI SetConsoleCtrlHandler(PHANDLER_ROUTINE HandlerRoutine, BOOL Add);
__declspec(dllimport) HANDLE WINAPI CreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPVOID lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
__declspec(dllimport) BOOL WINAPI CloseHandle(HANDLE hObject);
__declspec(dllimport) USHORT WINAPI RtlCaptureStackBackTrace(DWORD FramesToSkip, DWORD FramesToCapture, PVOID* BackTrace, DWORD* BackTraceHash);

/* PSAPI functions for enumerating modules (manual) */
typedef struct _MODULEINFO {
    void* lpBaseOfDll;
    DWORD SizeOfImage;
    void* EntryPoint;
} MODULEINFO;
__declspec(dllimport) BOOL WINAPI EnumProcessModules(HANDLE hProcess, void* lphModule, DWORD cb, LPDWORD lpcbNeeded);
__declspec(dllimport) BOOL WINAPI GetModuleInformation(HANDLE hProcess, void* hModule, MODULEINFO* pmi, DWORD cb);
__declspec(dllimport) DWORD WINAPI GetModuleFileNameExA(HANDLE hProcess, void* hModule, LPSTR lpFilename, DWORD nSize);
__declspec(dllimport) DWORD WINAPI GetModuleBaseNameA(HANDLE hProcess, void* hModule, LPSTR lpBaseName, DWORD nSize);

/* IMAGEHLP / DbgHelp types and functions (manual, x64-focused) */
#define IMAGE_FILE_MACHINE_I386   0x014c
#define IMAGE_FILE_MACHINE_AMD64  0x8664

#define SYMOPT_CASE_INSENSITIVE   0x00000001
#define SYMOPT_UNDNAME            0x00000002
#define SYMOPT_DEFERRED_LOADS     0x00000004
#define SYMOPT_LOAD_LINES         0x00000010

typedef enum _ADDRESS_MODE { AddrMode1616, AddrMode1632, AddrModeReal, AddrModeFlat } ADDRESS_MODE;
typedef struct _ADDRESS64 { DWORD64 Offset; WORD Segment; ADDRESS_MODE Mode; } ADDRESS64;
typedef struct _KDHELP64 {
    DWORD64 Thread; DWORD64 ThCallbackStack; DWORD64 ThCallbackBStore; DWORD64 NextCallback; DWORD64 FramePointer;
    DWORD64 KiCallUserMode; DWORD64 KeUserCallbackDispatcher; DWORD64 SystemRangeStart; DWORD64 KiUserExceptionDispatcher;
    DWORD64 UnusedAlignment; DWORD64 Reserved[4];
} KDHELP64;
typedef struct _STACKFRAME64 {
    ADDRESS64 AddrPC; ADDRESS64 AddrReturn; ADDRESS64 AddrFrame; ADDRESS64 AddrStack; ADDRESS64 AddrBStore;
    PVOID FuncTableEntry; DWORD64 Params[4]; BOOL Far; BOOL Virtual; DWORD64 Reserved[3]; KDHELP64 KdHelp;
} STACKFRAME64, *LPSTACKFRAME64;

#define MAX_SYM_NAME 1024

typedef struct _SYMBOL_INFO {
    ULONG SizeOfStruct;
    ULONG TypeIndex;
    ULONG64 Reserved[2];
    ULONG64 Index;
    ULONG64 Size;
    ULONG64 ModBase;
    ULONG Flags;
    ULONG64 Value;
    ULONG64 Address;
    ULONG Register;
    ULONG Scope;
    ULONG Tag;
    ULONG NameLen;
    ULONG MaxNameLen;
    char Name[1];
} SYMBOL_INFO, *PSYMBOL_INFO;

typedef struct _IMAGEHLP_LINE64 {
    DWORD SizeOfStruct;
    PVOID Key;
    DWORD LineNumber;
    LPSTR FileName;
    DWORD64 Address;
} IMAGEHLP_LINE64, *PIMAGEHLP_LINE64;

/* DbgHelp prototypes (manual). You may want to dynamically load these in future. */
__declspec(dllimport) BOOL IMAGEAPI SymInitialize(HANDLE hProcess, LPCSTR UserSearchPath, BOOL fInvadeProcess);
__declspec(dllimport) BOOL IMAGEAPI SymCleanup(HANDLE hProcess);
__declspec(dllimport) DWORD IMAGEAPI SymSetOptions(DWORD SymOptions);
__declspec(dllimport) BOOL IMAGEAPI SymFromAddr(HANDLE hProcess, DWORD64 Address, DWORD64* Displacement, PSYMBOL_INFO Symbol); /* SymFromAddr uses SYMBOL_INFO */
__declspec(dllimport) BOOL IMAGEAPI SymGetLineFromAddr64(HANDLE hProcess, DWORD64 qwAddr, DWORD* pdwDisplacement, PIMAGEHLP_LINE64 Line);
__declspec(dllimport) PVOID IMAGEAPI SymFunctionTableAccess64(HANDLE hProcess, DWORD64 AddrBase);
__declspec(dllimport) DWORD64 IMAGEAPI SymGetModuleBase64(HANDLE hProcess, DWORD64 Addr);
__declspec(dllimport) BOOL IMAGEAPI SymGetSymFromAddr64(HANDLE hProcess, DWORD64 qwAddr, DWORD64* pdwDisplacement, void* Symbol); /* legacy */

/* StackWalk */
typedef BOOL (CALLBACK *PREAD_PROCESS_MEMORY_ROUTINE64)(HANDLE, DWORD64, PVOID, DWORD, LPDWORD);
typedef PVOID (CALLBACK *PFUNCTION_TABLE_ACCESS_ROUTINE64)(HANDLE, DWORD64);
typedef DWORD64 (CALLBACK *PGET_MODULE_BASE_ROUTINE64)(HANDLE, DWORD64);
typedef DWORD64 (CALLBACK *PTRANSLATE_ADDRESS_ROUTINE64)(HANDLE, HANDLE, DWORD64);
__declspec(dllimport) BOOL IMAGEAPI StackWalk64(
    DWORD MachineType,
    HANDLE hProcess,
    HANDLE hThread,
    LPSTACKFRAME64 StackFrame,
    PVOID ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
    PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
    PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
    PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress);

/* Optional: SymLoadModule64 */
__declspec(dllimport) DWORD64 IMAGEAPI SymLoadModule64(HANDLE hProcess, void* hFile, LPCSTR ImageName, LPCSTR ModuleName, DWORD64 BaseOfDll, DWORD SizeOfDll);

/* Optional helper to get NT headers; minimal declaration (VERIFY) */
typedef struct _IMAGE_NT_HEADERS {
    // Minimal: We only need FileHeader.Machine. Put small structs to reach that field.
    struct {
        WORD Machine;
        WORD NumberOfSections;
        DWORD TimeDateStamp;
    } FileHeader;
} IMAGE_NT_HEADERS;
__declspec(dllimport) IMAGE_NT_HEADERS* WINAPI ImageNtHeader(void* Base); /* VERIFY: some toolchains provide this */

/* Implementation: get module list and load symbol modules */
static void load_process_modules_and_register_symbols(HANDLE process) {
    DWORD cbNeeded = 0;
    /* Query modules twice like common pattern */
    EnumProcessModules(process, NULL, 0, &cbNeeded);
    if (cbNeeded == 0) return;
    /* allocate buffer for handles */
    void* *modules = (void**)malloc(cbNeeded);
    if (!modules) return;
    if (!EnumProcessModules(process, modules, cbNeeded, &cbNeeded)) { free(modules); return; }
    int count = cbNeeded / sizeof(void*);
    char filename[4096];
    char modulename[512];
    for (int i = 0; i < count; ++i) {
        MODULEINFO mi;
        if (!GetModuleInformation(process, modules[i], &mi, sizeof(mi))) continue;
        /* get full path and base name */
        GetModuleFileNameExA(process, modules[i], filename, sizeof(filename));
        GetModuleBaseNameA(process, modules[i], modulename, sizeof(modulename));
        /* register module with dbghelp */
        SymLoadModule64(process, NULL, filename, modulename, (DWORD64)(uintptr_t)mi.lpBaseOfDll, mi.SizeOfImage);
    }
    free(modules);
}

/* Correctly prepare SYMBOL_INFO buffer */
static void prepare_symbol_info_buffer(char *buffer, PSYMBOL_INFO si) {
    /* buffer must be at least sizeof(SYMBOL_INFO) + (MAX_SYM_NAME-1) */
    memset(buffer, 0, sizeof(SYMBOL_INFO) + MAX_SYM_NAME);
    si->MaxNameLen = MAX_SYM_NAME;
    /* set SizeOfStruct to include the Name flexible array space as required by SymFromAddr */
    si->SizeOfStruct = (ULONG)(sizeof(SYMBOL_INFO) + (MAX_SYM_NAME - 1) * sizeof(char));
}

/* Resolve single address and print function + file:line (if available). We print pointer also */
static void resolve_and_print_win(DWORD64 addr, unsigned idx) {
    HANDLE proc = GetCurrentProcess();
    char buf_sym[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
    PSYMBOL_INFO si = (PSYMBOL_INFO)buf_sym;
    prepare_symbol_info_buffer(buf_sym, si);
    DWORD64 disp = 0;
    if (SymFromAddr(proc, addr, &disp, si)) {
        /* Print name + offset + pointer (pointer printing is optional; remove if undesired) */
        logf("%02u: %s + 0x%llx [0x%llx]", idx, si->Name, (unsigned long long)disp, (unsigned long long)addr);
    } else {
        DWORD err = (DWORD)GetLastError();
        DWORD64 modBase = SymGetModuleBase64(proc, addr);
        logf("%02u: [0x%llx] (SymFromAddr failed err=%lu, module_base=0x%llx)", idx, (unsigned long long)addr, (unsigned long)err, (unsigned long long)modBase);
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

/* The main Windows exception handler function (to be used in __except) */
LONG WINAPI CrashHandlerException(void* ep_void) {
    /* ep_void is EXCEPTION_POINTERS* */
    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();
    /* Try to avoid installing symbols if debugger present */
    /* You can decide to return EXCEPTION_CONTINUE_SEARCH if IsDebuggerPresent() (not declared here) */

    logf("%s: Program crashed\n", __FUNCTION__);

    /* Initialize dbghelp */
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
    if (!SymInitialize(process, NULL, 0)) {
        logf("SymInitialize failed (err=%lu)\n", (unsigned long)GetLastError());
    } else {
        /* load modules */
        load_process_modules_and_register_symbols(process);
    }

    /* Extract context and do StackWalk64 */
    /* ep_void is actually EXCEPTION_POINTERS*, but we don't declare full struct; just access as needed */
    struct { PVOID ExceptionRecord; PVOID ContextRecord; } *ep = (void*)ep_void;
    CONTEXT *ctx = (CONTEXT*)ep->ContextRecord;

    STACKFRAME64 frame;
    memset(&frame, 0, sizeof(frame));
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Mode = AddrModeFlat;

#if defined(_M_X64) || defined(__x86_64__)
    frame.AddrPC.Offset = ctx->Rip;
    frame.AddrStack.Offset = ctx->Rsp;
    frame.AddrFrame.Offset = ctx->Rbp;
    DWORD imageType = IMAGE_FILE_MACHINE_AMD64;
#else
    /* 32-bit minimal support (VERIFY) */
    frame.AddrPC.Offset = (DWORD64)0; /* placeholder */
    frame.AddrStack.Offset = (DWORD64)0;
    frame.AddrFrame.Offset = (DWORD64)0;
    DWORD imageType = IMAGE_FILE_MACHINE_I386;
#endif

    logf("Dumping backtrace:\n");
    unsigned i = 0;
    while (StackWalk64(imageType, process, thread, &frame, ctx, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL)) {
        if (frame.AddrPC.Offset == 0) break;
        resolve_and_print_win(frame.AddrPC.Offset, i++);
        if (i > 256) break;
    }

    SymCleanup(process);
    /* Let OS continue default handling */
    return 1; /* EXCEPTION_CONTINUE_SEARCH would be 0; adjust if you want */
}

/* Windows export to install handler via SetUnhandledExceptionFilter */
void install_crash_handler_windows(void) {
    /* Install unhandled exception filter to our CrashHandlerException wrapper */
    SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)CrashHandlerException);
    /* Optionally set console handler etc. (omitted) */
}

#endif /* PLATFORM_WINDOWS */

/* -------------------- LINUX IMPLEMENTATION (pure C) -------------------- */
#if defined(PLATFORM_LINUX)

#include <execinfo.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

/* Helper: run addr2line and capture first line of output (file:line), return 0 on success */
static int addr2line_for_exec(const char *exe_path, const char *addr_offset_hex, char *out_buf, size_t out_sz) {
    /* Build command: addr2line -e <exe_path> -f -p <offset>  (we use -f -p to get function+file:line)
       We'll use popen to read output. */
    char cmd[4096];
    /* Quote exe_path minimally (not robust against quotes). This is simple and workable. */
    snprintf(cmd, sizeof(cmd), "addr2line -e \"%s\" -f -p %s", exe_path, addr_offset_hex);
    FILE *fp = popen(cmd, "r");
    if (!fp) return -1;
    if (fgets(out_buf, (int)out_sz, fp) == NULL) {
        pclose(fp);
        return -1;
    }
    /* trim newline */
    size_t len = strlen(out_buf);
    if (len && out_buf[len-1] == '\n') out_buf[len-1] = '\0';
    pclose(fp);
    return 0;
}

/* Get executable path into out[sz] (Linux /proc method) */
static int get_exec_path(char *out, size_t sz) {
    char linkbuf[64];
    snprintf(linkbuf, sizeof(linkbuf), "/proc/%d/exe", (int)getpid());
    ssize_t n = readlink(linkbuf, out, sz - 1);
    if (n < 0) return -1;
    if (n >= (ssize_t)sz) n = sz - 1;
    out[n] = '\0';
    return 0;
}

/* Crash handler for Linux signals */
static void handle_crash_linux(int sig) {
    void *bt[256];
    int size = backtrace(bt, 256);
    char exe_path[4096] = {0};
    get_exec_path(exe_path, sizeof(exe_path));

    logf("%s: Program crashed with signal %d\n", __FUNCTION__, sig);
    logf("Dumping backtrace.\n");

    char **strings = backtrace_symbols(bt, size);
    if (!strings) {
        logf("-- no backtrace symbols --\n");
        return;
    }

    for (int i = 1; i < size; ++i) {
        /* backtrace_symbols output typically contains "binary(+0xoffset) [0xaddr]" or similar.
         * We will try to extract the +0xoffset part; fallback to printing the pointer.
         */
        char *sym = strings[i];
        /* find '+' and ')' */
        char *p_plus = NULL;
        char *p_paren = NULL;
        for (char *p = sym; *p; ++p) {
            if (*p == '+') { p_plus = p; break; }
        }
        for (char *p = sym; *p; ++p) {
            if (*p == ')') { p_paren = p; break; }
        }
        char offset_buf[64] = {0};
        int got_offset = 0;
        if (p_plus && p_paren && p_paren > p_plus) {
            size_t offlen = (size_t)(p_paren - p_plus - 1);
            if (offlen < sizeof(offset_buf)) {
                memcpy(offset_buf, p_plus + 1, offlen);
                offset_buf[offlen] = '\0';
                got_offset = 1;
            }
        }
        if (got_offset) {
            char resolved[1024] = {0};
            if (addr2line_for_exec(exe_path, offset_buf, resolved, sizeof(resolved)) == 0) {
                logf("[%d] %s\n", i, resolved);
            } else {
                /* fallback to raw symbol */
                logf("[%d] %s\n", i, sym);
            }
        } else {
            logf("[%d] %s\n", i, sym);
        }
    }

    free(strings);
    logf("-- END OF BACKTRACE --\n");
    /* abort to let OS handle the rest */
    abort();
}

/* Install / disable utilities for Linux */
void crash_handler_install(void) {
    signal(SIGSEGV, handle_crash_linux);
    signal(SIGFPE, handle_crash_linux);
    signal(SIGILL, handle_crash_linux);
}

void crash_handler_disable(void) {
    signal(SIGSEGV, SIG_DFL);
    signal(SIGFPE, SIG_DFL);
    signal(SIGILL, SIG_DFL);
}

#endif /* PLATFORM_LINUX */