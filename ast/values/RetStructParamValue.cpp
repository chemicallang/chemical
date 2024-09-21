// Copyright (c) Qinetik 2024.

#include "RetStructParamValue.h"
#include "ast/types/PointerType.h"
#include "ast/types/VoidType.h"

//hybrid_ptr<BaseType> RetStructParamValue::get_base_type() {
//    return hybrid_ptr<BaseType> { create_type().release(), true };
//}

BaseType* RetStructParamValue::create_type(ASTAllocator& allocator) {
    return new (allocator.allocate<PointerType>()) PointerType(new (allocator.allocate<VoidType>()) VoidType(nullptr), nullptr);
}