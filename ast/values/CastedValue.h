// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 01/03/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/base/BaseType.h"

class CastedValue : public Value {
public:

    CastedValue(std::unique_ptr<Value> value, std::unique_ptr<BaseType> type);

    Value *copy() override;

    std::unique_ptr<BaseType> create_type() const override {
        return std::unique_ptr<BaseType>(type->copy());
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    void link(SymbolResolver &linker) override;

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Value *llvm_value(Codegen &gen) override;

#endif

    ValueType value_type() const override {
        return type->value_type();
    }

    BaseTypeKind type_kind() const override {
        return type->kind();
    }

    std::string representation() const override;

    std::unique_ptr<Value> value;
    std::unique_ptr<BaseType> type;
};