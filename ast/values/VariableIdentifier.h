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
    Value *find_in(InterpretScope &scope, Value *parent) override {
        return parent->child(value);
    }

    void set_value_in(InterpretScope &scope, Value *parent, Value *next_value, Operation op) override {
#ifdef DEBUG
        if (parent == nullptr) {
            scope.error("set_value_in in variable identifier, received null pointer to parent");
        }
#endif
        parent->set_child_value(value, next_value, op);
    }

    void set_identifier_value(InterpretScope &scope, Value *newValue, Operation op) override {
        auto it = scope.find_value_iterator(value);
        if (it.first == it.second.end()) {
            if(op == Operation::Assignment) {
                // value has been declared above and now is being assigned
                scope.declare(value, newValue);
            } else {
                scope.error("couldn't set non-existent variable " + value + " with operation " + to_string(op));
            }
            return;
        }
        auto v = it.first->second;

        auto nextValue = op == Operation::Assignment ? (newValue) : (
                scope.global->expr_evaluators[
                        ExpressionEvaluator::index(v->value_type(), v->value_type(), op)
                ](v, newValue)
        );

        if (v->primitive()) {
            delete v;
        }
        it.first->second = nextValue;
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
        auto found = scope.find_value(value);
        if (found != nullptr) {
            return found;
        } else {
            return nullptr;
        }
    }

    /**
     * every identifier's value will be moved to new owner at return
     */
    Value* return_value(InterpretScope& scope) override {
        // current identifier, holds the value, we find it
        auto val = scope.find_value_iterator(value);
        // check if not found
        if (val.first == val.second.end() || val.first->second == nullptr) {
            return nullptr;
        }
        // store the pointer to the value
        auto store = val.first->second;
        // delete the previous owner
        val.first->second = nullptr;
        // return the pointer
        return store;
    }

    Value *param_value(InterpretScope &scope) override {
        // evaluates the value, if its primitive copies it
        // otherwise, we pass another reference to the value, in the function calls
        auto val = scope.find_value_iterator(value);
        if (val.first == val.second.end() || val.first->second == nullptr) {
            return nullptr;
        }
        if (val.first->second->primitive()) {
            return val.first->second->copy();
        } else {
            return val.first->second;
        }
    }

    bool is_initializer_reference(InterpretScope &scope) override {
        // true, because we don't value to be destroyed, since its an identifier
        // it was probably defined above in the scope
        return true;
    }

    Value *initializer_value(InterpretScope &scope) override {
        // user is trying to do var x = y;
        // where y is this variable identifier
        auto val = scope.find_value_iterator(value);
        if (val.first == val.second.end() || val.first->second == nullptr) {
            return nullptr;
        }
        // we'll create a copy if its primitive & create reference if it's not
        if (val.first->second->primitive()) {
            return val.first->second->copy();
        } else {
            return val.first->second;
        }
    }

    Value* assignment_value(InterpretScope &scope) override {
        // user is trying to do var x = y;
        // where y is this variable identifier
        auto val = scope.find_value_iterator(value);
        if (val.first == val.second.end() || val.first->second == nullptr) {
            return nullptr;
        }
        // we'll create a copy if its primitive & create reference if it's not
        if (val.first->second->primitive()) {
            return val.first->second->copy();
        } else {
            return val.first->second;
        }
    }

    std::string representation() const override {
        return value;
    }

private:
    std::string value; ///< The string value.

};