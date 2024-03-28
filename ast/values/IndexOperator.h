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
    llvm::Value* elem_pointer(Codegen& gen, ASTNode* arr) {
        return gen.builder->CreateGEP(arr->llvm_type(gen), arr->llvm_pointer(gen), {value->llvm_value(gen)});;
    }

    llvm::Value * llvm_pointer(Codegen &gen) override {
        auto arr = resolve(gen);
        return elem_pointer(gen, arr);
    }

    llvm::Value * llvm_value(Codegen &gen) override {
        auto resolved = resolve(gen);
        return gen.builder->CreateLoad(resolved->llvm_elem_type(gen), elem_pointer(gen, resolved), "arr0");
    }
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