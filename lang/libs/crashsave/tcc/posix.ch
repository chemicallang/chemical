@extern
public func tcc_backtrace(thing : *char, _ : any...) : int

func segfault_handler(a : int) {
    signal(SIGSEGV, SIG_DFL);
    signal(SIGABRT, SIG_DFL);
    signal(SIGFPE,  SIG_DFL);
    signal(SIGILL,  SIG_DFL);
    tcc_backtrace("Backtrace")
}

public func install_crash_handler(exe_path : *char, onCrash : () => void) {
    signal(SIGSEGV, segfault_handler);
    signal(SIGABRT, segfault_handler);
    signal(SIGFPE,  segfault_handler);
    signal(SIGILL,  segfault_handler);
}