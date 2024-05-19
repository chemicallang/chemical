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

    NotValue(std::unique_ptr<Value> value) : value(std::move(value)) {}

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    void link(SymbolResolver &linker) override;

    bool primitive() override;

    std::string representation() const override;

#ifdef COMPILER_BUILD

    llvm::Value *llvm_value(Codegen &gen) override;

#endif

    ValueType value_type() const override {
        return value->value_type();
    }

    BaseTypeKind type_kind() const override {
        return value->type_kind();
    }

    std::unique_ptr<BaseType> create_type() const override;

    std::unique_ptr<Value> value;

};