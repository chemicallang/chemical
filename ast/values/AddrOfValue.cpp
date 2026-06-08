// Copyright (c) Chemical Language Foundation 2025.

#include "AddrOfValue.h"
#include "ast/base/ASTNode.h"
#include "ast/types/ReferenceType.h"
#include "ast/structures/FunctionParam.h"
#include "ReferenceOfValue.h"
#include "compiler/lab/TargetData.h"

void AddrOfValue::determine_type() {
    const auto valueType = value->getType();
    const auto can = valueType->canonical();
    getType()->type = can->kind() == BaseTypeKind::Reference ? can->as_reference_type_unsafe()->type : valueType;
}

uint64_t AddrOfValue::byte_size(TargetData& target) {
    return target.is64Bit ? 8 : 4;
}

void ReferenceOfValue::determine_type() {
    const auto valueType = value->getType();
    getType()->type = valueType;
}

uint64_t ReferenceOfValue::byte_size(TargetData& target) {
    return target.is64Bit ? 8 : 4;
}