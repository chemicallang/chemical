// Copyright (c) Qinetik 2024.

#include "Assignment.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

void AssignStatement::code_gen(Codegen &gen) {
    if (assOp == Operation::Assignment) {
        gen.builder->CreateStore(value->llvm_value(gen), lhs->llvm_pointer(gen));
    } else {
        auto loaded = lhs->llvm_value(gen);
        auto operated = gen.operate(assOp, loaded, value->llvm_value(gen));
        gen.builder->CreateStore(operated, lhs->llvm_pointer(gen));
    }
}

#endif