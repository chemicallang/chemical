@extern
public func tcc_backtrace(thing : *char, _ : any...) : int

// TODO: shift these from here to cstd
comptime const SIGINT =          2   // interrupt
comptime const SIGILL =          4   // illegal instruction - invalid function image
comptime const SIGFPE =          8   // floating point exception
comptime const SIGSEGV =         11  // segment violation
comptime const SIGTERM =         15  // Software termination signal from kill
comptime const SIGBREAK =        21  // Ctrl-Break sequence
comptime const SIGABRT =         22  // abnormal termination triggered by abort call

func segfault_handler(a : int) {
    tcc_backtrace("Backtrace")
}

public func install_crash_handler() {
    signal(SIGSEGV, segfault_handler);
    signal(SIGABRT, segfault_handler);
    signal(SIGFPE,  segfault_handler);
    signal(SIGILL,  segfault_handler);
}