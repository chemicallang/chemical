// Copyright (c) Qinetik 2024.

#include "IntNumValue.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"
#include "compiler/Codegen.h"

llvm::Type *IntNumValue::llvm_type(Codegen &gen) {
    return gen.builder->getIntNTy(get_num_bits());
}

llvm::Value *IntNumValue::llvm_value(Codegen &gen) {
    return gen.builder->getIntN(get_num_bits(), get_num_value());
}

#endif