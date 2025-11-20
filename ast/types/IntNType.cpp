// Copyright (c) Chemical Language Foundation 2025.

#include "IntNType.h"
#include "ast/types/BoolType.h"
#include "ast/base/InterpretScope.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/base/TypeBuilder.h"
#include "compiler/lab/TargetData.h"
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

unsigned int IntNType::num_bits(TargetData& target) const noexcept {
    switch(_kind) {
        case IntNTypeKind::Char:
        case IntNTypeKind::UChar:
        case IntNTypeKind::I8:
        case IntNTypeKind::U8:
            return 8;
        case IntNTypeKind::Short:
        case IntNTypeKind::UShort:
        case IntNTypeKind::I16:
        case IntNTypeKind::U16:
            return 16;
        case IntNTypeKind::Int:
        case IntNTypeKind::UInt:
        case IntNTypeKind::I32:
        case IntNTypeKind::U32:
            return 32;
        case IntNTypeKind::Long:
        case IntNTypeKind::ULong:
            if(target.is64Bit) {
                if(target.windows) {
                    return 32;
                } else {
                    return 64;
                }
            } else {
                return 32;
            }
        case IntNTypeKind::LongLong:
        case IntNTypeKind::ULongLong:
        case IntNTypeKind::I64:
        case IntNTypeKind::U64:
            return 64;
        case IntNTypeKind::Int128:
        case IntNTypeKind::UInt128:
            return 128;
        default:
#ifdef DEBUG
            abort();
#endif
            return 0;
    }
}

uint64_t IntNType::byte_size(TargetData& target) {
    switch(_kind) {
        case IntNTypeKind::Char:
        case IntNTypeKind::UChar:
        case IntNTypeKind::I8:
        case IntNTypeKind::U8:
            return 1;
        case IntNTypeKind::Short:
        case IntNTypeKind::UShort:
        case IntNTypeKind::I16:
        case IntNTypeKind::U16:
            return 2;
        case IntNTypeKind::Int:
        case IntNTypeKind::UInt:
        case IntNTypeKind::I32:
        case IntNTypeKind::U32:
            return 4;
        case IntNTypeKind::Long:
        case IntNTypeKind::ULong:
            if(target.is64Bit) {
                if(target.windows) {
                    return 4;
                } else {
                    return 8;
                }
            } else {
                return 4;
            }
        case IntNTypeKind::I64:
        case IntNTypeKind::U64:
        case IntNTypeKind::LongLong:
        case IntNTypeKind::ULongLong:
            return 8;
        case IntNTypeKind::Int128:
        case IntNTypeKind::UInt128:
            return 16;
        default:
    #ifdef DEBUG
            abort();
    #endif
            return 0;
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

bool IntNType::satisfies(Value *value, bool assignment) {
    const auto literal = value->isValueIntegerLiteral();
    auto otherType = value->getType();
    if(!otherType) return false;
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