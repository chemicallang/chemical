// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 01/03/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/types/BoolType.h"

class BoolValue : public Value {
public:

    bool value;

    /**
     * constructor
     */
    constexpr BoolValue(
            bool value,
            BoolType* boolTy,
            SourceLocation location
    ) : Value(ValueKind::Bool, boolTy, location), value(value) {

    }

    BoolType* getType() {
        return (BoolType*) Value::getType();
    }

    BoolValue *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<BoolValue>()) BoolValue(value, getType(), encoded_location());
    }

    uint64_t byte_size(bool is64Bit) final {
        return 1;
    }

    BaseType* known_type() final {
        return (BaseType*) &BoolType::instance;
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

#endif

};