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

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Value *llvm_value(Codegen &gen) override;

#endif

    std::string representation() const override;

private:
    std::unique_ptr<Value> value;
    std::unique_ptr<BaseType> type;
};