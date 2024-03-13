// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 02/03/2024.
//

#pragma once

#include <utility>

#include "ast/base/Value.h"
#include <llvm/IR/ValueSymbolTable.h>

/**
 * @brief Class representing a VariableIdentifier.
 */
class VariableIdentifier : public Value {
public:

    /**
     * @brief Construct a new VariableIdentifier object.
     *
     * @param value The string value.
     */
    VariableIdentifier(std::string value) : value(std::move(value)) {}

    // will find value by this name in the parent
    Value * find_in(Value *parent) override {
        return parent->child(value);
    }

    void set_identifier_value(InterpretScope &scope, Value *newValue) override {
        auto it = scope.find(value);
        if (!it.first) {
            std::cerr << "Couldn't set variable " << value
                      << " as there's no such variable in parent, or previous value doesn't exist ";
            return;
        }
        auto v = it.second->second;
        if (v->primitive()) {
            delete v;
        }
        it.second->second = newValue;
    }

    void set_identifier_value(InterpretScope &scope, Value *newValue, Operation op) override {
        auto it = scope.find(value);
        if (!it.first) {
            std::cerr << "Couldn't set variable " << value
                      << " as there's no such variable in parent, or previous value doesn't exist ";
            return;
        }
        auto v = it.second->second;

        // creating an expression
        auto nextValue = ExpressionEvaluator::functionVector[
                ExpressionEvaluator::index(v->value_type(), v->value_type(), op)
        ](v, newValue);

        if (v->primitive()) {
            delete v;
        }
        it.second->second = nextValue;
    }

    llvm::Value * llvm_pointer(Codegen &gen) override {
        auto v = gen.builder->GetInsertBlock()->getValueSymbolTable()->lookup(value);
        if(v == nullptr) {
            gen.error("Couldn't find variable identifier : " + value);
            return nullptr;
        }
        return v;
    }

    ASTNode* resolve(Codegen &gen) {
        auto found = gen.current.find(value);
        if(gen.current.end() == found) {
            gen.error("Couldn't find variable identifier in scope : " + value);
            throw std::runtime_error("couldn't find variable identifier in scope : " + value);
        } else {
            return found->second;
        }
    }

    llvm::Value * llvm_value(Codegen &gen) override {
        auto v = llvm_pointer(gen);
        return gen.builder->CreateLoad(resolve(gen)->llvm_type(gen), v, value);
    }

    Value *evaluated_value(InterpretScope &scope) override {
        auto found = scope.find(value);
        if (found.first) {
            return found.second->second;
        } else {
            return nullptr;
        }
    }

    std::string representation() const override {
        return value;
    }

private:
    std::string value; ///< The string value.

};