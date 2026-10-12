#include "core/main/CompilerMain.h"

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
    return compiler_main(argc, argv);
}