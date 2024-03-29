// Copyright (c) Qinetik 2024.

#include "VarInit.h"

#ifdef COMPILER_BUILD
#include "compiler/llvmimpl.h"

void VarInitStatement::code_gen(Codegen &gen) {
    if (value.has_value()) {
        allocaInst = value.value()->llvm_allocate(gen, identifier);
    } else {
        allocaInst = gen.builder->CreateAlloca(llvm_type(gen), nullptr, identifier);
    }
}

#endif