// Global state
var g_bt_state : *mut backtrace_state = null;
var g_user_callback : (a : int, stack : std::span<StackFrame>) => void = null;

// --- Callbacks for libbacktrace ---
func error_callback(data : *mut void, msg : *char, errnum : int) {
    fprintf(get_stderr(), "libbacktrace error: %s (%d)\n", msg, errnum);
}

func full_callback(
    data : *mut void, pc : uintptr_t,
    filename : *char, lineno : int,
    function  :*char
) : int {
    var frames = data as *std::vector<StackFrame>;
    var frame = StackFrame();
    frame.pc = pc;
    frame.filename = if(filename) filename else "??";
    frame.lineno = lineno;
    frame.function = if(function) function else "??";
    frames.push(frame);
    return 0; // continue
}

// --- Signal handler ---
func crash_handler(sig : int) {
    fprintf(get_stderr(), "\n*** Crash detected (signal %d) ***\n", sig);

    var frames = std::vector<StackFrame>();
    if (g_bt_state) {
        backtrace_full(g_bt_state, 1, full_callback, error_callback, &frames);
    }

    // Print to stderr
    for (auto &f : frames) {
        fprintf(get_stderr(), "  at %s:%d: %s (0x%lx)\n",
                f.filename.data(), f.lineno,
                f.function.data(), (long)f.pc);
    }

    // Call user callback
    if (g_user_callback) {
        g_user_callback(sig, std::span<StackFrame>(frames));
    }

    // _exit(1); // optionally call terminate immediately
}

// --- Public API ---
void install_crash_handler(argv0 : *char, on_crash : (a : int, stack : std::span<StackFrame>) => void) {
    g_bt_state = backtrace_create_state(argv0, 1, error_callback, nullptr);
    g_user_callback = std::move(on_crash);

    // Cross-platform signals
    std::signal(SIGSEGV, crash_handler);
    std::signal(SIGABRT, crash_handler);
// #if defined(SIGBUS)
//     std::signal(SIGBUS, crash_handler);
// #endif
// #if defined(SIGILL)
//     std::signal(SIGILL, crash_handler);
// #endif
// #if defined(SIGFPE)
//     std::signal(SIGFPE, crash_handler);
// #endif
}