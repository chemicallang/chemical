// Copyright (c) Qinetik 2024.

#include "IntNType.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

llvm::Type *IntNType::llvm_type(Codegen &gen) const {
    auto ty = gen.builder->getIntNTy(number);
    if(!ty) {
        gen.error("Couldn't get intN type for int:" + std::to_string(number));
    }
    return ty;
}

#endif