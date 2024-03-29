// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 29/02/2024.
//

#pragma once

#include <vector>
#include <memory>
#include "ast/base/Value.h"
#include "ast/base/ASTNode.h"
#include "ast/structures/FunctionDeclaration.h"

class FunctionCall : public Value {
public:

    FunctionCall(std::string name, std::vector<std::unique_ptr<Value>> values) : name(std::move(name)),
                                                                                 values(std::move(values)) {

    }

    FunctionCall(FunctionCall &&other) = delete;

    void link(ASTLinker &linker) override;

    ASTNode *linked_node(ASTLinker &linker) override;

    ASTNode* find_link_in_parent(ASTNode *parent) override;

    bool primitive() override {
        return false;
    }

    Value *find_in(InterpretScope &scope, Value *parent) override {
        return parent->call_member(scope, name, values);
    }

    Value *evaluated_value(InterpretScope &scope) override {
        if (definition != nullptr) {
            return definition->call(&scope, values);
        } else {
            scope.error("(function call) calling a function that is not found or has no body, name : " + name);
        }
        return nullptr;
    }

    Value *copy(InterpretScope &scope) override {
        scope.error("copy called on function call");
        return nullptr;
    }

    Value *initializer_value(InterpretScope &scope) override {
        return evaluated_value(scope);
    }

    Value *assignment_value(InterpretScope &scope) override {
        return evaluated_value(scope);
    }

    Value *param_value(InterpretScope &scope) override {
        return evaluated_value(scope);
    }

    Value *return_value(InterpretScope &scope) override {
        return evaluated_value(scope);
    }

    void interpret(InterpretScope &scope) override {
        auto value = evaluated_value(scope);
        if (value != nullptr && value->primitive()) {
            delete value;
        }
    }

#ifdef COMPILER_BUILD
    llvm::Value * llvm_value(Codegen &gen) override;

    void code_gen(Codegen &gen) override;
#endif

    std::string representation() const override {
        std::string rep;
        rep.append(name);
        rep.append(1, '(');
        int i = 0;
        while (i < values.size()) {
            rep.append(values[i]->representation());
            if (i != values.size() - 1) {
                rep.append(1, ',');
            }
            i++;
        }
        rep.append(");");
        return rep;
    }

    std::string name;
    std::vector<std::unique_ptr<Value>> values;
    FunctionDeclaration *definition;

};