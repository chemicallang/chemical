// Copyright (c) Qinetik 2024.

#include "StructType.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

llvm::Type *StructType::llvm_type(Codegen &gen) const {
    std::vector<llvm::Type *> types;
    for (auto &elem: elem_types) {
        types.emplace_back(elem->llvm_type(gen));
    }
    return llvm::StructType::get(*gen.ctx, types);
}

#endif