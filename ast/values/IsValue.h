// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 01/03/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/types/BoolType.h"

class IsValue : public Value {
public:

    std::unique_ptr<Value> value;
    std::unique_ptr<BaseType> type;
    bool is_negating;

    IsValue(std::unique_ptr<Value> value, std::unique_ptr<BaseType> type, bool is_negating = false);

    IsValue *copy() override;

    /**
     * std::nullopt means unknown, true or false means it evaluated
     */
    std::optional<bool> get_comp_time_result();

    hybrid_ptr<BaseType> get_base_type() override {
        return hybrid_ptr<BaseType> { (BaseType*) &BoolType::instance, false };
    }

    BaseType* known_type() override {
        return (BaseType*) &BoolType::instance;
    }

    std::unique_ptr<BaseType> create_type() override {
        return std::unique_ptr<BaseType>(new BoolType());
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    void link(SymbolResolver &linker, std::unique_ptr<Value>& value_ptr) override;

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

};