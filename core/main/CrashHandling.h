#pragma once

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Registers the crash handler.
 *
 * Call this once at the beginning of your program to set up the crash
 * signal/exception handlers.
 */
void register_crash_handler(void);

#ifdef __cplusplus
}
#endif

#if defined(_WIN32)

#include <windows.h>
#include <dbghelp.h>
#include <stdlib.h>

/* Link against the DbgHelp library */
#pragma comment(lib, "dbghelp.lib")

static LONG WINAPI windows_exception_handler(EXCEPTION_POINTERS *pExceptionInfo) {
    /* A handle to the current process. */
    HANDLE process = GetCurrentProcess();
    /* A handle to the current thread. */
    HANDLE thread = GetCurrentThread();

    /* Buffer for the symbol information. */
    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
    SYMBOL_INFO *pSymbol = (SYMBOL_INFO *)buffer;

    /* Buffer for the line information. */
    IMAGEHLP_LINE64 line;
    DWORD displacement;

    /* Initialize the symbol handler for the current process. */
    SymInitialize(process, NULL, TRUE);

    CONTEXT context = *pExceptionInfo->ContextRecord;

    STACKFRAME64 stack;
    memset(&stack, 0, sizeof(STACKFRAME64));

#ifdef _M_IX86
    stack.AddrPC.Offset = context.Eip;
    stack.AddrPC.Mode = AddrModeFlat;
    stack.AddrStack.Offset = context.Esp;
    stack.AddrStack.Mode = AddrModeFlat;
    stack.AddrFrame.Offset = context.Ebp;
    stack.AddrFrame.Mode = AddrModeFlat;
#elif _M_X64
    stack.AddrPC.Offset = context.Rip;
    stack.AddrPC.Mode = AddrModeFlat;
    stack.AddrStack.Offset = context.Rsp;
    stack.AddrStack.Mode = AddrModeFlat;
    stack.AddrFrame.Offset = context.Rbp;
    stack.AddrFrame.Mode = AddrModeFlat;
#else
#error "Platform not supported"
#endif

    fprintf(stderr, "stacktrace:\n");

    for (int frame_num = 0; ; ++frame_num) {
        /* Walk the stack. */
        BOOL result = StackWalk64(
            IMAGE_FILE_MACHINE_AMD64,
            process,
            thread,
            &stack,
            &context,
            NULL,
            SymFunctionTableAccess64,
            SymGetModuleBase64,
            NULL
        );

        if (!result) {
            break;
        }

        pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        pSymbol->MaxNameLen = MAX_SYM_NAME;

        /* Get symbol (function name) from the address. */
        if (SymFromAddr(process, stack.AddrPC.Offset, 0, pSymbol)) {
            /* Attempt to get line number information. */
            if (SymGetLineFromAddr64(process, stack.AddrPC.Offset, &displacement, &line)) {
                fprintf(stderr, "%d: '%s' at %s:%lu\n", frame_num, pSymbol->Name, line.FileName, line.LineNumber);
            } else {
                fprintf(stderr, "%d: '%s' at 0x%0llX\n", frame_num, pSymbol->Name, pSymbol->Address);
            }
        } else {
            fprintf(stderr, "%d: <unknown function> at 0x%0llX\n", frame_num, stack.AddrPC.Offset);
        }
    }

    /* Clean up the symbol handler. */
    SymCleanup(process);

    /* Let the default handler take over to terminate the program. */
    return EXCEPTION_CONTINUE_SEARCH;
}

void register_crash_handler(void) {
    SetUnhandledExceptionFilter(windows_exception_handler);
}


#elif defined(__GNUC__) || defined(__clang__)

#include <signal.h>
#include <execinfo.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

/* For dladdr() to get symbol information */
#if defined(__linux__) || defined(__APPLE__)
#include <dlfcn.h>
#endif

#define MAX_STACK_FRAMES 128

static void posix_signal_handler(int sig, siginfo_t *siginfo, void *context) {
    (void)context; /* Unused parameter */

    fprintf(stderr, "Caught signal %d (%s)\n", sig, strsignal(sig));
    fprintf(stderr, "stacktrace:\n");

    void *buffer[MAX_STACK_FRAMES];
    int nptrs = backtrace(buffer, MAX_STACK_FRAMES);

    for (int i = 0; i < nptrs; i++) {
        const char *function_name = "<unknown function>";
        const char *file_name = "<unknown file>";

#if defined(__linux__) || defined(__APPLE__)
        Dl_info info;
        if (dladdr(buffer[i], &info) && info.dli_sname) {
            if (info.dli_sname[0] == '_') {
                /* On some systems, symbols have a leading underscore. Skip it. */
                function_name = info.dli_sname + 1;
            } else {
                function_name = info.dli_sname;
            }
            if (info.dli_fname) {
                file_name = info.dli_fname;
            }
        }
#endif
        fprintf(stderr, "%d: '%s' at %s\n", i, function_name, file_name);
    }

    /* Restore default signal handler and re-raise signal to get default behavior (e.g., core dump) */
    signal(sig, SIG_DFL);
    raise(sig);
}

void register_crash_handler(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = posix_signal_handler;
    sa.sa_flags = SA_SIGINFO;

    /* Register for critical signals */
    sigaction(SIGSEGV, &sa, NULL); /* Segmentation fault */
    sigaction(SIGFPE, &sa, NULL);  /* Floating-point exception */
    sigaction(SIGILL, &sa, NULL);  /* Illegal instruction */
    sigaction(SIGBUS, &sa, NULL);  /* Bus error */
    sigaction(SIGABRT, &sa, NULL); /* Abort signal */
}

#else

/* For unsupported platforms, provide a stub implementation. */
void register_crash_handler(void) {
    fprintf(stderr, "Crash handler not supported on this platform.\n");
}

#endif