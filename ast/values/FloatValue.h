// Copyright (c) Qinetik 2024.

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
    /**
     * @brief Construct a new FloatValue object.
     *
     * @param value The floating-point value.
     */
    FloatValue(float value) : value(value) {}

    uint64_t byte_size(bool is64Bit) const {
        return 4;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Value *llvm_value(Codegen &gen) override;

#endif

    float as_float() override {
        return value;
    }

    std::unique_ptr<BaseType> create_type() const override {
        return std::make_unique<FloatType>();
    }

    Value *copy() override {
        return new FloatValue(value);
    }

    ValueType value_type() const override {
        return ValueType::Float;
    }

    BaseTypeKind type_kind() const override {
        return BaseTypeKind::Float;
    }

    float value; ///< The floating-point value.
};