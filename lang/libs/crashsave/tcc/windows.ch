@extern
public func tcc_backtrace(thing : *char, _ : any...) : int

func segfault_handler(a : int) {
    tcc_backtrace("Backtrace")
}

public func install_crash_handler() {
    signal(SIGSEGV, segfault_handler);
    signal(SIGABRT, segfault_handler);
    signal(SIGFPE,  segfault_handler);
    signal(SIGILL,  segfault_handler);
}