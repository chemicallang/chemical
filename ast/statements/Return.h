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
    ReturnStatement(std::optional<std::unique_ptr<Value>> value) : value(std::move(value)) {}

    void interpret(InterpretScope &scope) override {
        auto current = &scope;
        while(current != nullptr && !current->node->supportsReturn()) {
            current = current->parent;
        }
        if(current == nullptr) {
            scope.error("invalid return statement, couldn't find returnable node up in the tree");
            return;
        }
        if(value.has_value()) {
            current->node->set_return(value->get()->evaluated_value(scope));
        } else {
            current->node->set_return(nullptr);
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
    std::optional<std::unique_ptr<Value>> value;

};