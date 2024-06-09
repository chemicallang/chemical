// Copyright (c) Qinetik 2024.

#ifdef FEAT_BITCODE_GEN

#include "compiler/Codegen.h"
#include <llvm/Bitcode/BitcodeWriter.h>

void Codegen::save_as_bc_file(const std::string &out_path) {

    setup_for_target();

    std::error_code EC;
    llvm::raw_fd_ostream dest(out_path, EC, sys::fs::OF_None);

    if (EC) {
        error("Could not open file: " + EC.message());
        return;
    }

    WriteBitcodeToFile(*module, dest);

    dest.flush();
    dest.close();

}

#endif