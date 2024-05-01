// Copyright (c) Qinetik 2024.

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
    /**
     * @brief Construct a new DoubleValue object.
     *
     * @param value The double value.
     */
    DoubleValue(double value) : value(value) {}

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Value *llvm_value(Codegen &gen) override;

#endif

    std::unique_ptr<BaseType> create_type() const override {
        return std::make_unique<DoubleType>();
    }

    Value *copy() override {
        return new DoubleValue(value);
    }

    double as_double() override {
        return value;
    }

    std::string representation() const override {
        std::string rep;
        rep.append(std::to_string(value));
        return rep;
    }

    ValueType value_type() const override {
        return ValueType::Double;
    }

    BaseTypeKind type_kind() const override {
        return BaseTypeKind::Double;
    }

private:
    double value; ///< The double value.
};