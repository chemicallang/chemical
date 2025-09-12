// Copyright (c) Chemical Language Foundation 2025.

#include "IntNType.h"
#include "ast/types/BoolType.h"
#include "ast/base/InterpretScope.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/base/TypeBuilder.h"
#include "ast/types/ReferenceType.h"
#include "ast/values/IntNumValue.h"
#include "ast/values/DoubleValue.h"
#include "ast/values/FloatValue.h"
#include "ast/values/BoolValue.h"
#include "NullPtrType.h"

bool BoolType::satisfies(BaseType *type) {
    switch(type->kind()) {
        case BaseTypeKind::Bool:
            return true;
        case BaseTypeKind::Reference:
            return type->as_reference_type_unsafe()->type->kind() == BaseTypeKind::Bool;
        default:
            return false;
    }
}

Value* IntNType::create(
        ASTAllocator& allocator,
        TypeBuilder& typeBuilder,
        uint64_t value,
        SourceLocation loc
) {
    return new (allocator.allocate<IntNumValue>()) IntNumValue(
        value,
        this,
        loc
    );
}

bool IntNType::satisfies(BaseType *given) {
    const auto type = given->canonical();
    return type->kind() == BaseTypeKind::IntN && satisfies(type->as_intn_type_unsafe());
}

bool IntNType::satisfies(ASTAllocator &allocator, Value *value, bool assignment) {
    const auto literal = value->isValueIntegerLiteral();
    auto otherType = value->getType();
    if(!otherType) return false;
    if(otherType->kind() == BaseTypeKind::Reference) {
        otherType = otherType->as_reference_type_unsafe()->type->canonical();
    }
    if(literal && otherType->kind() == BaseTypeKind::IntN) {
        return true;
    } else {
        return satisfies(otherType);
    }
}

double get_double_value(Value* value, ValueKind k) {
    if(k == ValueKind::Double) {
        return (double) ((DoubleValue*) value)->value;
    } else if(k == ValueKind::Float) {
        return (double) ((FloatValue*) value)->value;
    } else if(k == ValueKind::IntN) {
        return (double) ((IntNumValue*) value)->get_num_value();
    } else {
        return 0;
    }
}

Value* pack_by_kind(InterpretScope& scope, ValueKind kind, double value, SourceLocation location) {
    auto& typeBuilder = scope.global->typeBuilder;
    switch(kind) {
        case ValueKind::Double:
            return new (scope.allocate<DoubleValue>()) DoubleValue((double) value, typeBuilder.getDoubleType(), location);
        case ValueKind::Float:
            return new (scope.allocate<FloatValue>()) FloatValue((float) value, typeBuilder.getFloatType(), location);
        case ValueKind::Bool:
            return new (scope.allocate<BoolValue>()) BoolValue((int) value, typeBuilder.getBoolType(), location);
        default:
            return nullptr;
    }
}

Value* pack_by_kind(InterpretScope& scope, IntNTypeKind kind, uint64_t value, SourceLocation location) {
    auto& typeBuilder = scope.global->typeBuilder;
    return new (scope.allocate<IntNumValue>()) IntNumValue(value, typeBuilder.getIntNType(kind), location);
}