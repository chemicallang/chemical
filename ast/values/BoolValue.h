// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 01/03/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/types/BoolType.h"

class BoolValue : public Value, public BoolType {
public:

    /**
     * @brief Construct a new CharValue object.
     *
     * @param value The character value.
     */
    BoolValue(bool value) : value(value) {}

    BoolValue *copy() override {
        return new BoolValue(value);
    }

    uint64_t byte_size(bool is64Bit) override {
        return 1;
    }

    hybrid_ptr<BaseType> get_base_type() override {
        return hybrid_ptr<BaseType> { this, false };
    }

    std::unique_ptr<BaseType> create_type() override {
        return std::make_unique<BoolType>();
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) override;

#endif

    [[nodiscard]]
    ValueType value_type() const override {
        return ValueType::Bool;
    }

    [[nodiscard]]
    BaseTypeKind type_kind() const override {
        return BaseTypeKind::Bool;
    }

    bool as_bool() override {
        return value;
    }


    bool value;

};