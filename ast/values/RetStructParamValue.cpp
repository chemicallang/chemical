// Copyright (c) Qinetik 2024.

#include "RetStructParamValue.h"
#include "ast/types/PointerType.h"
#include "ast/types/VoidType.h"

hybrid_ptr<BaseType> RetStructParamValue::get_base_type() {
    return hybrid_ptr<BaseType> { create_type().release(), true };
}

std::unique_ptr<BaseType> RetStructParamValue::create_type() {
    return std::make_unique<PointerType>(std::make_unique<VoidType>(nullptr), nullptr);
}