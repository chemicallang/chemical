// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 01/03/2024.
//

#pragma once

#include <memory>
#include "ast/base/Value.h"

// A value that's preceded by a not operator !value
class NotValue : public Value {
public:

    std::unique_ptr<Value> value;
    CSTToken* token;

    explicit NotValue(std::unique_ptr<Value> value, CSTToken* token) : value(std::move(value)), token(token) {}

    CSTToken* cst_token() override {
        return token;
    }

    ValueKind val_kind() override {
        return ValueKind::NotValue;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    bool link(SymbolResolver &linker, std::unique_ptr<Value>& value_ptr) override;

    bool primitive() override;

#ifdef COMPILER_BUILD

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) override;

#endif

    NotValue* copy() override {
        return new NotValue(std::unique_ptr<Value>(value->copy()), token);
    }

    [[nodiscard]]
    ValueType value_type() const override {
        return value->value_type();
    }

    [[nodiscard]]
    BaseTypeKind type_kind() const override {
        return value->type_kind();
    }

    std::unique_ptr<BaseType> create_type() override;

    hybrid_ptr<BaseType> get_base_type() override;

    BaseType* known_type() override;

};