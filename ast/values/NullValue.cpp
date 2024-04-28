// Copyright (c) Qinetik 2024.

#include "NullValue.h"
#include "ast/types/PointerType.h"
#include "ast/types/VoidType.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

llvm::Value *NullValue::llvm_value(Codegen &gen) {
    auto ptrType = llvm::PointerType::get(llvm::IntegerType::get(*gen.ctx, 32), 0);
    return llvm::ConstantPointerNull::get(ptrType);
}

#endif


std::unique_ptr<BaseType> NullValue::create_type() const {
    return std::make_unique<PointerType>(std::make_unique<VoidType>());
}