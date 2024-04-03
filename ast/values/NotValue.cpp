// Copyright (c) Qinetik 2024.

#include "NotValue.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

llvm::Value *NotValue::llvm_value(Codegen &gen) {
    return gen.builder->CreateNot(value->llvm_value(gen));
}

#endif

void NotValue::link(SymbolResolver &linker) {
    value->link(linker);
}

bool NotValue::primitive() {
    return false;
}

std::string NotValue::representation() const {
    std::string rep;
    rep.append(1, '!');
    rep.append(value->representation());
    return rep;
}