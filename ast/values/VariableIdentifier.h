// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 02/03/2024.
//

#pragma once

#include <utility>
#include "ast/structures/FunctionDeclaration.h"
#include "ast/base/Value.h"
#ifdef COMPILER_BUILD
#include <llvm/IR/ValueSymbolTable.h>
#endif
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
    Value *find_in(Value *parent) override {
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

#ifdef COMPILER_BUILD
    llvm::Value *arg_value(Codegen &gen, ASTNode* node) {
        auto param = node->as_parameter();
        if (param != nullptr && gen.current_function != nullptr) {
            for (const auto &arg: gen.current_function->args()) {
                if (arg.getArgNo() == param->index) {
                    return (llvm::Value *) &arg;
                }
//                else {
//                    gen.error("no mismatch" + std::to_string(arg.getArgNo()) + " " + std::to_string(param->index));
//                }
            }
        }
//        else {
//            gen.error("param or function missing");
//        }
        return nullptr;
    }

    llvm::AllocaInst* llvm_alloca(Codegen &gen) {
        auto found = gen.allocated.find(value);
        if(found == gen.allocated.end()) {
            gen.error("llvm_alloca called on variable identifier, couldn't locate the identifier : " + value);
            return nullptr;
        } else {
            return found->second;
        }
    }

    llvm::Value *llvm_pointer(Codegen &gen) override {
        return llvm_alloca(gen);
    }

    llvm::Value *llvm_value(Codegen &gen) override {
        auto resolved = resolve(gen);
        auto argVal = arg_value(gen, resolved);
        if (argVal != nullptr) {
            return argVal;
        }
        auto v = llvm_pointer(gen);
        return gen.builder->CreateLoad(resolved->llvm_type(gen), v, value);
    }

    ASTNode *resolve(Codegen &gen) {
        auto found = gen.current.find(value);
        if (gen.current.end() == found) {
            gen.error("Couldn't find variable identifier in scope : " + value);
            throw std::runtime_error("couldn't find variable identifier in scope : " + value);
        } else {
            return found->second;
        }
    }
#endif

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