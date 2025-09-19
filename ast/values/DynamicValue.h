// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"
#include "ast/base/TypeLoc.h"
#include "ast/types/DynamicType.h"

/**
 * the dynamic value is created when you write dyn<Type>(expr)
 */
class DynamicValue : public Value {
public:

    TypeLoc type;
    Value* value;

    /**
     * constructor
     */
    DynamicValue(
            TypeLoc type,
            Value* value,
            SourceLocation location,
            DynamicType* dynType
    ) : Value(ValueKind::DynamicValue, dynType, location), type(type), value(value) {

    }

    DynamicType* getType() const noexcept {
        return (DynamicType*) Value::getType();
    }

#ifdef COMPILER_BUILD

    llvm::Value* llvm_pointer(Codegen &gen) override;

    llvm::Value* llvm_value(Codegen &gen, BaseType *type) override;

    llvm::AllocaInst* llvm_allocate(Codegen &gen, const std::string &identifier, BaseType *expected_type) override;

#endif

};