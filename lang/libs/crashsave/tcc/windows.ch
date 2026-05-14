@extern
public func tcc_backtrace(thing : *char, _ : any...) : int

@extern
public func _exit(status : int) : void

var g_crashing : int = 0
var g_on_crash : () => void = null

func segfault_handler(a : int) : void {
    if (g_crashing) {
        _exit(1);
    }
    g_crashing = 1;
    if (g_on_crash) {
        g_on_crash();
    }
    tcc_backtrace("Backtrace")
    abort();
}

public func install_crash_handler(exe_path : *char, onCrash : () => void) {
    g_on_crash = onCrash;
    signal(SIGSEGV, segfault_handler);
    signal(SIGFPE, segfault_handler);
    signal(SIGILL, segfault_handler);
}
