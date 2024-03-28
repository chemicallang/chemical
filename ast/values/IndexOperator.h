// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 29/02/2024.
//

#pragma once

#include <memory>
#include <utility>
#include "ast/base/Value.h"

class IndexOperator : public Value {
public:

    IndexOperator(std::string identifier, std::unique_ptr<Value> value) : identifier(std::move(identifier)),
                                                                          value(std::move(value)) {

    }

    bool primitive() override {
        return false;
    }

    Value *find_in(InterpretScope& scope, Value *parent) override {
#ifdef DEBUG
      try {
          return parent->index(scope, value->evaluated_value(scope)->as_int());
      }  catch (...) {
          std::cerr << "[InterpretError] index operator only support's integer indexes at the moment";
      }
#endif
        parent->index(scope, value->evaluated_value(scope)->as_int());
        return nullptr;
    }

#ifdef COMPILER_BUILD
    ASTNode* resolve(Codegen& gen) {
        auto resolved = gen.current.find(identifier);
        if(resolved == gen.current.end()) {
            gen.error("Couldn't find identifier for indexing operator : " + identifier);
            throw std::runtime_error("Couldn't find identifier for indexing operator : " + identifier);
        } else {
            return resolved->second;
        }
    }

    // TODO isInBounds optimization, when we know that index is in bounds
    llvm::Value* elem_pointer(Codegen& gen, ASTNode* arr);

    llvm::Value * llvm_pointer(Codegen &gen) override;

    llvm::Value * llvm_value(Codegen &gen) override;

#endif

    std::string representation() const override {
        std::string rep;
        rep.append(1, '[');
        rep.append(value->representation());
        rep.append(1, ']');
        return rep;
    }

    std::string identifier;
    std::unique_ptr<Value> value;

};