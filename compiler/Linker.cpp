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

LLD_HAS_DRIVER(coff)
LLD_HAS_DRIVER(elf)
LLD_HAS_DRIVER(mingw)
LLD_HAS_DRIVER(macho)
LLD_HAS_DRIVER(wasm)

int lld_main(int argc, char **argv, const llvm::ToolContext &) {

    sys::Process::UseANSIEscapeCodes(true);

    ArrayRef<const char *> args(argv, argv + argc);

    auto result = lld::lldMain(args, llvm::outs(), llvm::errs(), LLD_ALL_DRIVERS);

    return result.retCode;

}

int Codegen::invoke_lld(const std::vector<std::string> &command_args) {
    // Convert the vector of strings to an ArrayRef<const char *>
    std::vector<const char *> args_cstr;
    args_cstr.reserve(command_args.size() + 1);
    std::string lld_driver;
    auto triple = llvm::Triple(sys::getDefaultTargetTriple());
    if (triple.isOSDarwin())
        lld_driver = "ld64.lld";
    else if (triple.isOSWindows())
        lld_driver = "lld-link";
    else
        lld_driver = "ld.lld";
    args_cstr.push_back(lld_driver.c_str());
    for (const std::string& arg : command_args) {
        args_cstr.push_back(arg.c_str());
    }
    // invocation
    ToolContext context{};
    return lld_main(args_cstr.size(), const_cast<char**>(args_cstr.data()), context);
}

void Codegen::link_objs_as_exes_lld(std::vector<std::string>& obj_files, const std::string &out_path, const std::vector<std::string>& linker_flags) {

    // Determine the lld driver
    std::string lld_driver;
    auto triple = llvm::Triple(sys::getDefaultTargetTriple());
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
    int argc = object_files_cstr.size() + 3 + linker_flags.size(); // Plus 3 for program name, output file, and terminator
    std::vector<const char *> argv(argc);
    argv[0] = lld_driver.c_str(); // Use the correct lld driver
    for (size_t i = 0; i < object_files_cstr.size(); ++i) {
        argv[i + 1] = object_files_cstr[i];
    }

    // Add linker flags
    int arg_index = object_files_cstr.size() + 1;
    for (const auto& flag : linker_flags) {
        argv[arg_index++] = flag.c_str();
    }

    argv[arg_index++] = "-o";
    argv[arg_index++] = out_path.c_str(); // Output executable path
    argv[arg_index] = nullptr; // Null terminator

    // Tool context (optional, can be left empty)
    llvm::ToolContext toolContext{};

    // Call lld_main
    int result = lld_main(argc, const_cast<char**>(argv.data()), toolContext);

    if (result != 0) {
        error("Error: Failed to link object files into executable\n");
    }

}

#endif