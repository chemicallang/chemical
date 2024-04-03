// Copyright (c) Qinetik 2024.

#ifdef COMPILER_BUILD
#include "Codegen.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include <string>
#include <system_error>

using namespace llvm;
using namespace llvm::sys;

TargetMachine * Codegen::setup_for_target(const std::string &TargetTriple) {

    // Initialize the target registry etc.
    InitializeAllTargetInfos();
    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeAllAsmParsers();
    InitializeAllAsmPrinters();

    std::string Error = "unknown error related to target lookup";
    auto Target = TargetRegistry::lookupTarget(TargetTriple, Error);

    // Print an error and exit if we couldn't find the requested target.
    // This generally occurs if we've forgotten to initialise the
    // TargetRegistry or we have a bogus target triple.
    if (!Target) {
        error(Error);
        return nullptr;
    }

    auto CPU = "generic";
    auto Features = "";

    TargetOptions opt;
    auto RM = std::optional<Reloc::Model>();
    auto TheTargetMachine = Target->createTargetMachine(
            TargetTriple, CPU, Features, opt, RM);

    module->setDataLayout(TheTargetMachine->createDataLayout());
    module->setTargetTriple(TargetTriple);

    return TheTargetMachine;

}

void save_as_file_type(Codegen* gen, const std::string &out_path, llvm::CodeGenFileType type) {

    auto TheTargetMachine = gen->setup_for_target();
    if(TheTargetMachine == nullptr) {
        return;
    }

    std::error_code EC;
    raw_fd_ostream dest(out_path, EC, sys::fs::OF_None);

    if (EC) {
        gen->error("Could not open file: " + EC.message());
        return;
    }

    legacy::PassManager pass;

    if (TheTargetMachine->addPassesToEmitFile(pass, dest, nullptr, type)) {
        gen->error("TheTargetMachine can't emit a file of this type");
        return;
    }

    pass.run(*gen->module);
    dest.flush();

}

#ifdef FEAT_ASSEMBLY_GEN

/**
 * saves as assembly file to this path
 * @param TargetTriple
 */
void Codegen::save_to_assembly_file(const std::string &out_path) {
    save_as_file_type(this, out_path, llvm::CodeGenFileType::CGFT_AssemblyFile);
}

#endif

/**
 * saves as object file to this path
 * @param out_path
 */
void Codegen::save_to_object_file(const std::string &out_path) {
    save_as_file_type(this, out_path, llvm::CodeGenFileType::CGFT_ObjectFile);
}

#ifdef FEAT_BITCODE_GEN

#include <llvm/Bitcode/BitcodeWriter.h>

void Codegen::save_as_bc_file(const std::string &out_path) {

    setup_for_target();

    std::error_code EC;
    raw_fd_ostream dest(out_path, EC, sys::fs::OF_None);

    if (EC) {
        error("Could not open file: " + EC.message());
        return;
    }

    WriteBitcodeToFile(*module, dest);

    dest.flush();
    dest.close();

}

#endif

#endif