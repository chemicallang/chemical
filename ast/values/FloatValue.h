// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/types/FloatType.h"

/**
 * @brief Class representing a floating-point value.
 */
class FloatValue : public Value {
public:

    float value; ///< The floating-point value.

    /**
     * constructor
     */
    constexpr FloatValue(
        float value,
        SourceLocation location
    ) : Value(ValueKind::Float, location), value(value) {

    }


//    hybrid_ptr<BaseType> get_base_type() final {
//        return hybrid_ptr<BaseType> { (BaseType*) &FloatType::instance, false };
//    }

    BaseType* known_type() final {
        return (BaseType*) &FloatType::instance;
    }

    uint64_t byte_size(bool is64Bit) {
        return 4;
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

#endif

    BaseType* create_type(ASTAllocator &allocator) final {
        return new (allocator.allocate<FloatType>()) FloatType(encoded_location());
    }

    FloatValue *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<FloatValue>()) FloatValue(value, encoded_location());
    }

};