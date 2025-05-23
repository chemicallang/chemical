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
        SourceLocation location
    ) : Value(ValueKind::Bool, location), value(value) {

    }


    BoolValue *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<BoolValue>()) BoolValue(value, encoded_location());
    }

    uint64_t byte_size(bool is64Bit) final {
        return 1;
    }

    BaseType* known_type() final {
        return (BaseType*) &BoolType::instance;
    }

    BaseType* create_type(ASTAllocator& allocator) final {
        return new (allocator.allocate<BoolType>()) BoolType();
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

#endif

};