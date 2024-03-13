// Copyright (c) Qinetik 2024.

#include "Codegen.h"

llvm::Type *Codegen::llvm_type(const std::optional<std::string> &type) {
    if (type.has_value()) {
        if (type.value() == "int") {
            return builder->getInt32Ty();
        } else if (type.value() == "string") {
            return builder->getInt8PtrTy();
        } else {
            error("UNKNOWN FUNCTION RETURN TYPE : " + type.value());
            return nullptr;
        }
    } else {
        return builder->getVoidTy();
    }
}