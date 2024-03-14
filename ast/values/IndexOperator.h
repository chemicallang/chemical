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

    IndexOperator(std::string identifier, std::unique_ptr<Value> value) : identifier(std::move(identifier)), value(std::move(value)) {

    }

    ASTNode* resolve(Codegen& gen) {
        auto resolved = gen.current.find(identifier);
        if(resolved == gen.current.end()) {
            gen.error("Couldn't find identifier for indexing operator : " + identifier);
            throw std::runtime_error("Couldn't find identifier for indexing operator : " + identifier);
        } else {
            return resolved->second;
        }
    }

    llvm::Value* elem_pointer(Codegen& gen, ASTNode* arr) {
        return gen.builder->CreateGEP(arr->llvm_type(gen), arr->llvm_pointer(gen), {llvm::ConstantInt::get(llvm::Type::getInt32Ty(*gen.ctx), 0), value->llvm_value(gen)});;
    }

    llvm::Value * llvm_pointer(Codegen &gen) override {
        auto arr = resolve(gen);
        return elem_pointer(gen, arr);
    }

    llvm::Value * llvm_value(Codegen &gen) override {
        auto resolved = resolve(gen);
        return gen.builder->CreateLoad(resolved->llvm_elem_type(gen), elem_pointer(gen, resolved), "arr0");
    }

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