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

    bool reference() override {
        return true;
    }

    void set_value_in(InterpretScope &scope, Value *parent, Value *next_value, Operation op) override {
#ifdef DEBUG
        if (parent == nullptr) {
            scope.error("set_value_in in variable identifier, received null pointer to parent");
        }
#endif
        parent->set_child_value(value, next_value, op);
    }

    void set_identifier_value(InterpretScope &scope, Value *rawValue, Operation op) override {

        auto newValue = rawValue->assignment_value(scope);
        if (newValue == nullptr) {
            scope.error("trying to assign null ptr to identifier " + value);
            return;
        }

        // var init statement of current value, being assigned var x (<--- this one) = y
        auto var_init = declaration(scope);
        if (var_init == nullptr) {
            scope.error("couldn't find declaration for identifier " + value);
            return;
        }

        // making one statement a reference
        if (rawValue->reference() && !newValue->primitive()) {
            // var init statement of value that owns the value var x = y (<---- this one)
            auto value_var_init = rawValue->declaration(scope);
            if (value_var_init == nullptr) {
                scope.error("couldn't find declaration of the identifier " + rawValue->representation() +
                            " to assign to " + value);
                return;
            }
#ifdef DEBUG
            if(var_init->position == value_var_init->position) {
                scope.error("two init statements have the same position in AST, first : " + var_init->representation() + " second : " + value_var_init->representation() + ", triggered by assignment between their variables");
            }
#endif
            if (var_init->position > value_var_init->position) {
                // going down, declared above and now being assigned to declaration below (take reference / make decl below a reference)
                var_init->is_reference = true;
            } else {
                // going up, declared below and now being assigned to declaration above (move the value / make decl above a reference)
                value_var_init->is_reference = true;
                var_init->is_reference = false;
            }
        } else {
            var_init->is_reference = false;
        }

        // iterator for previous value
        auto itr = scope.find_value_iterator(value);

        auto nextValue = newValue;

        // previous value doesn't exist, so it must only be a declaration above that is being assigned
        // preventing x += 1 (requires previous value)
        if (op != Operation::Assignment) {

            // previous value iterator is required
            if (itr.first == itr.second.end()) {
                scope.error("couldn't set non-existent variable " + value + " with operation " + to_string(op));
                return;
            }

            // get the previous value, perform operation on it
            auto prevValue = itr.first->second;
            nextValue = scope.global->expr_evaluators[
                    ExpressionEvaluator::index(prevValue->value_type(), prevValue->value_type(), op)
            ](prevValue, newValue);

        }

        // delete previous value if its a primitive / not a reference & exists
        if(itr.first != itr.second.end() && (!var_init->is_reference || itr.first->second->primitive())) {
            delete itr.first->second;
        }

        if(itr.first == itr.second.end()) {
            var_init->declare(nextValue);
        } else {
            itr.first->second = nextValue;
        }

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

    VarInitStatement *declaration(InterpretScope &scope) override {
        auto node = scope.find_node(value);
        if (node != nullptr) {
            return node->as_var_init();
        }
        return nullptr;
    }

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
    Value *return_value(InterpretScope &scope) override {
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
            auto store = val.first->second;
            val.first->second = nullptr;
            return store;
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

    Value *assignment_value(InterpretScope &scope) override {
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