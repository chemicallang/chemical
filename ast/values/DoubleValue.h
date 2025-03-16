// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/types/DoubleType.h"

/**
 * @brief Class representing a double value.
 */
class DoubleValue : public Value {
public:

    double value; ///< The double value.

    /**
     * constructor
     */
    constexpr DoubleValue(
        double value,
        SourceLocation location
    ) : Value(ValueKind::Double, location), value(value) {}

    uint64_t byte_size(bool is64Bit) final {
        return 8;
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

#endif

    BaseType* create_type(ASTAllocator &allocator) final {
        return new (allocator.allocate<DoubleType>()) DoubleType(encoded_location());
    }

    BaseType* known_type() final {
        return (BaseType*) &DoubleType::instance;
    }

    DoubleValue *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<DoubleValue>()) DoubleValue(value, encoded_location());
    }

};