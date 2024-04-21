// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 01/03/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/base/BaseType.h"
#include "ast/types/PointerType.h"

class DereferenceValue : public Value {
public:

    DereferenceValue(std::unique_ptr<Value> value);

    Value *copy() override;

    std::unique_ptr<BaseType> create_type() const override;

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Value *llvm_value(Codegen &gen) override;

#endif

    void link(SymbolResolver &linker) override;

    std::string representation() const override;

private:
    std::unique_ptr<Value> value;
};