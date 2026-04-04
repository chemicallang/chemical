// Copyright (c) Chemical Language Foundation 2025.

#include "AddrOfValue.h"
#include "ast/base/ASTNode.h"
#include "ast/types/ReferenceType.h"
#include "ast/structures/FunctionParam.h"

void AddrOfValue::determine_type() {
    const auto valueType = value->getType();
    const auto can = valueType->canonical();
    _ptr_type.type = can->kind() == BaseTypeKind::Reference ? can->as_reference_type_unsafe()->type : valueType;
    // we set it here, since _ptr_type is invalid before it (the child type is nullptr)
    setType(&_ptr_type);
}