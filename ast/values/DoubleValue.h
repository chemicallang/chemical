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
            DoubleType* doubleTy,
            SourceLocation location
    ) : Value(ValueKind::Double, doubleTy, location), value(value) {}

    inline DoubleType* getType() noexcept {
        return (DoubleType*) Value::getType();
    }

    uint64_t byte_size(bool is64Bit) final {
        return 8;
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

#endif

    BaseType* create_type(ASTAllocator &allocator) final {
        return new (allocator.allocate<DoubleType>()) DoubleType();
    }

    BaseType* known_type() final {
        return (BaseType*) &DoubleType::instance;
    }

    DoubleValue *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<DoubleValue>()) DoubleValue(value, getType(), encoded_location());
    }

};