#include "core/main/CompilerMain.h"
#include "utils/LargeStack.h"
#include "utils/FaultReporter.h"

#if defined(COMPILER_BUILD)
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Signals.h"
#endif

/**
 * the main method of the compiler
 */
int main(int argc, char *argv[]) {
#if defined(COMPILER_BUILD)
    llvm::sys::PrintStackTraceOnErrorSignal(llvm::StringRef(argv[0]));
#endif
    // No-op unless the build enables CHEM_DEBUG_FAULT_REPORT (see
    // utils/FaultReporter.h). The lab re-registers it after TinyCC installs its
    // own vectored handler.
    chem::fault_reporter::refresh();
    // The front end (MIR lowering, comptime evaluation) recurses on the
    // compiler's own stack, and the comptime recursion guard treats the last
    // 512 KiB of the current thread's stack as reserved. A Windows process only
    // gets 1 MiB of stack by default, so run the whole compile on a thread with
    // a stack sized for a large module. The worker pool threads get the same
    // (see `ctpl.h`).
    int result = 0;
    chem::run_on_large_stack([&]() {
        result = compiler_main(argc, argv);
    });
    return result;
}
