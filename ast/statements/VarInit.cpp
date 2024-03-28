// Copyright (c) Qinetik 2024.

#include "VarInit.h"

#ifdef COMPILER_BUILD
#include "compiler/llvmimpl.h"

void VarInitStatement::code_gen(Codegen &gen) {
    declare(gen);
    if (value.has_value()) {
        value.value()->llvm_allocate(gen, identifier);
    } else {
        gen.allocated[identifier] = gen.builder->CreateAlloca(llvm_type(gen), nullptr, identifier);
    }
}

#endif