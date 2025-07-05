#define B_STACKTRACE_IMPL
#include "b_stacktrace.h"
#include <signal.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
// SEH handler must return a LONG
LONG WINAPI my_seh_handler(EXCEPTION_POINTERS* info);
#else
#include <signal.h>
  #include <string.h>
  #include <unistd.h>
  #include <stdlib.h>
  #include <execinfo.h>
  #include <stdio.h>
  void my_signal_handler(int sig, siginfo_t* si, void* unused);
#endif

static void register_crash_handlers();

#ifdef _WIN32

LONG WINAPI my_seh_handler(EXCEPTION_POINTERS* info) {
    // initialize symbols
    HANDLE process = GetCurrentProcess();
    SymInitialize(process, NULL, TRUE);

    // capture and print backtrace...
    // (use your b_stacktrace_get_string() or custom logic here)
    // e.g.:
    char* trace = b_stacktrace_get_string();
    fprintf(stderr, "SEH exception 0x%08X\n%s\n",
            info->ExceptionRecord->ExceptionCode,
            trace);
    free(trace);

    SymCleanup(process);
    return EXCEPTION_EXECUTE_HANDLER;
}

static void register_crash_handlers() {
    // First-chance handler (0 = call first)
    AddVectoredExceptionHandler(0, my_seh_handler);
}

#else  // POSIX

void my_signal_handler(int sig, siginfo_t* si, void* unused) {
    (void)si; (void)unused;
    void* stack[32];
    int n = backtrace(stack, 32);

    fprintf(stderr, "Caught signal %d (%s)\n", sig, strsignal(sig));
    char** symbols = backtrace_symbols(stack, n);
    for (int i = 0; i < n; i++) {
        fprintf(stderr, "  %s\n", symbols[i]);
    }
    free(symbols);
    _Exit(128 + sig);
}

static void register_crash_handlers() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = my_signal_handler;
    sa.sa_flags     = SA_SIGINFO | SA_RESTART;

    // list of signals to catch
    int signals[] = {
        SIGSEGV,  // invalid memory reference
        SIGABRT,  // abort()
        SIGFPE,   // floating-point exception
        SIGILL,   // illegal instruction
        SIGBUS,   // bus error
        SIGTRAP,  // breakpoint or trace trap
        SIGTERM,  // termination request
    };

    for (size_t i = 0; i < sizeof(signals)/sizeof(*signals); i++) {
        sigaction(signals[i], &sa, NULL);
    }
}

#endif  // _WIN32