// Copyright (c) Qinetik 2024.

#include "Negative.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

llvm::Value *NegativeValue::llvm_value(Codegen &gen) {
    return gen.builder->CreateNeg(value->llvm_value(gen));
}

#endif

void NegativeValue::link(ASTLinker &linker) {
    value->link(linker);
}

bool NegativeValue::primitive() {
    return false;
}

std::string NegativeValue::representation() const {
    std::string rep;
    rep.append(1, '-');
    rep.append(value->representation());
    return rep;
}