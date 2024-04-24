// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 01/03/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/base/BaseType.h"
#include "ast/types/PointerType.h"

class AddrOfValue : public Value {
public:

    AddrOfValue(std::unique_ptr<Value> value);

    Value *copy() override;

    std::unique_ptr<BaseType> create_type() const override {
        return std::make_unique<PointerType>(value->create_type());
    }

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Value *llvm_value(Codegen &gen) override;

#endif

    ValueType value_type() const override {
        return ValueType::Pointer;
    }

    BaseTypeKind type_kind() const override {
        return BaseTypeKind::Pointer;
    }

    void link(SymbolResolver &linker) override;

    std::string representation() const override;

private:
    std::unique_ptr<Value> value;
};