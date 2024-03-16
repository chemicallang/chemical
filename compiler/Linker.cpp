// Copyright (c) Qinetik 2024.

#ifdef COMPILER_BUILD

#include "Codegen.h"

#include "lld/Common/Driver.h"
#include "lld/Common/ErrorHandler.h"
#include "lld/Common/Memory.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/CrashRecoveryContext.h"
#include "llvm/Support/LLVMDriver.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/PluginLoader.h"
#include "llvm/Support/Process.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/TargetParser/Triple.h"
#include <cstdlib>
#include <optional>

using namespace lld;
using namespace llvm;
using namespace llvm::sys;

namespace lld {
    extern bool inTestOutputDisabled;

// Bypass the crash recovery handler, which is only meant to be used in
// LLD-as-lib scenarios.
    int unsafeLldMain(llvm::ArrayRef<const char *> args,
                      llvm::raw_ostream &stdoutOS, llvm::raw_ostream &stderrOS,
                      llvm::ArrayRef<DriverDef> drivers, bool exitEarly);
} // namespace lld

LLD_HAS_DRIVER(coff)
LLD_HAS_DRIVER(elf)
LLD_HAS_DRIVER(mingw)
LLD_HAS_DRIVER(macho)
LLD_HAS_DRIVER(wasm)

int lld_main(int argc, char **argv, const llvm::ToolContext &) {

    sys::Process::UseANSIEscapeCodes(true);

    if (::getenv("FORCE_LLD_DIAGNOSTICS_CRASH")) {
        llvm::errs()
                << "crashing due to environment variable FORCE_LLD_DIAGNOSTICS_CRASH\n";
        LLVM_BUILTIN_TRAP;
    }

    ArrayRef<const char *> args(argv, argv + argc);

    int r =
            lld::unsafeLldMain(args, llvm::outs(), llvm::errs(), LLD_ALL_DRIVERS,
                    /*exitEarly=*/true);
    return r;

}

void Codegen::link_object_files_as_executable(std::vector<std::string>& obj_files, const std::string &out_path) {

    // Determine the lld driver
    std::string lld_driver;
    auto triple = llvm::Triple(sys::getProcessTriple());
    if (triple.isOSDarwin())
        lld_driver = "ld64.lld";
    else if (triple.isOSWindows())
        lld_driver = "lld-link";
    else
        lld_driver = "ld.lld";

    // Convert object files to C-style array of char pointers
    std::vector<const char *> object_files_cstr;
    for (const auto &file : obj_files) {
        object_files_cstr.push_back(file.c_str());
    }

    // Arguments for lld_main
    int argc = object_files_cstr.size() + 3; // Plus 3 for program name, output file, and terminator
    std::vector<const char *> argv(argc);
    argv[0] = lld_driver.c_str(); // Use the correct lld driver
    for (size_t i = 0; i < object_files_cstr.size(); ++i) {
        argv[i + 1] = object_files_cstr[i];
    }
    argv[argc - 2] = "-o";
    argv[argc - 1] = out_path.c_str(); // Output executable path

    // Tool context (optional, can be left empty)
    llvm::ToolContext toolContext{};

    // Call lld_main
    int result = lld_main(argc, const_cast<char**>(argv.data()), toolContext);

    if (result != 0) {
        error("Error: Failed to link object files into executable\n");
    }

}

#endif