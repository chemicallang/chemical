// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 05/03/2024.
//

#pragma once

#include <memory>
#include "ast/base/Value.h"

// A value that's preceded by a negative operator -value
class NegativeValue : public Value {
public:

    NegativeValue(std::unique_ptr<Value> value) : value(std::move(value)) {}

    std::string representation() const override {
        std::string rep;
        rep.append(1, '-');
        rep.append(value->representation());
        return rep;
    }

#ifdef COMPILER_BUILD
    llvm::Value * llvm_value(Codegen &gen) override {
        return gen.builder->CreateNeg(value->llvm_value(gen));
    }
#endif

    std::unique_ptr<Value> value;

};