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
    explicit DoubleValue(double value) : value(value) {}

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    uint64_t byte_size(bool is64Bit) override {
        return 8;
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) override;

#endif

    std::unique_ptr<BaseType> create_type() override {
        return std::make_unique<DoubleType>();
    }

    hybrid_ptr<BaseType> get_base_type() override {
        return hybrid_ptr<BaseType> { (BaseType*) &DoubleType::instance, false };
    }

    BaseType* known_type() override {
        return (BaseType*) &DoubleType::instance;
    }

    DoubleValue *copy() override {
        return new DoubleValue(value);
    }

    double as_double() override {
        return value;
    }

    [[nodiscard]]
    ValueType value_type() const override {
        return ValueType::Double;
    }

    [[nodiscard]]
    BaseTypeKind type_kind() const override {
        return BaseTypeKind::Double;
    }

    double value; ///< The double value.
};