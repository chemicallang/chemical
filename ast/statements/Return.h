// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"

class ReturnStatement : public ASTNode {
public:

    /**
     * @brief Construct a new ReturnStatement object.
     */
    ReturnStatement(std::optional<std::unique_ptr<Value>> value, FunctionDeclaration* declaration) : value(std::move(value)), declaration(declaration) {}

    void interpret(InterpretScope &scope) override {
        if(value.has_value()) {
            declaration->set_return(value->get()->evaluated_value(scope));
        } else {
            declaration->set_return(nullptr);
        }
    }

    void code_gen(Codegen &gen) override {
        if(value.has_value()) {
            gen.CreateRet(value.value()->llvm_value(gen));
        } else {
            gen.CreateRet(nullptr);
        }
    }

    std::string representation() const override {
        std::string ret;
        ret.append("return");
        if(value.has_value()) {
            ret.append(1, ' ');
            ret.append(value.value()->representation());
            ret.append(1, ';');
        } else {
            ret.append(1, ';');
        }
        return ret;
    }

private:
    FunctionDeclaration* declaration;
    std::optional<std::unique_ptr<Value>> value;

};