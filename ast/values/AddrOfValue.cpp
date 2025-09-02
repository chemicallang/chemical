// Copyright (c) Chemical Language Foundation 2025.

#include "AddrOfValue.h"
#include "ast/base/ASTNode.h"
#include "ast/types/ReferenceType.h"
#include "ast/structures/FunctionParam.h"

BaseType* AddrOfValue::known_type() {
    if(!_ptr_type.type) {
        _ptr_type.type = value->known_type();
    }
    return &_ptr_type;
}

void AddrOfValue::determine_type() {
    const auto valueType = value->getType();
    const auto can = valueType->canonical();
    _ptr_type.type = can->kind() == BaseTypeKind::Reference ? can->as_reference_type_unsafe()->type : valueType;
}