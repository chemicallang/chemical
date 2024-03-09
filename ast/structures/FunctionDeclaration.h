// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/values/VariableIdentifier.h"

using func_params = std::vector<std::pair<std::string, std::string>>;

class FunctionDeclaration : public Value {
public:

    /**
     * @brief Construct a new FunctionDeclaration object.
     *
     * @param name The name of the function.
     * @param returnType The return type of the function.
     * @param parameters The parameters of the function.
     * @param body The body of the function.
     */
    FunctionDeclaration(
            std::string name,
            func_params params,
            std::optional<std::string> returnType
    ) : name(std::move(name)), params(std::move(params)), returnType(std::move(returnType)) {
        for (auto &param: this->params) {
            param.first.shrink_to_fit();
            param.second.shrink_to_fit();
        }
    }

    void interpret(InterpretScope &scope) override {
        scope.values[name] = this;
        declarationScope = &scope;
    }

    Value *call(std::vector<std::unique_ptr<Value>> &call_params) {
        InterpretScope child(declarationScope, declarationScope->global, &body, this);
        if (params.size() != call_params.size()) {
            child.error("function " + name + " requires " + std::to_string(params.size()) + ", but given params are " +
                        std::to_string(call_params.size()));
            return nullptr;
        }
        auto i = 0;
        while (i < params.size()) {
            // TODO all function values are being copied
            // TODO make sure only primitive values are copied
            child.values[params[i].first] = call_params[i]->copy();
            i++;
        }
        body.interpret(child);
        return interpretReturn;
    }

    // called by the return statement
    void set_return(Value *value) {
        interpretReturn = value;
    }

    FunctionDeclaration *as_function() override {
        return this;
    }

    std::string representation() const override {
        std::string ret;
        ret.append("func ");
        ret.append(name);
        ret.append(1, '(');
        int i = 0;
        while (i < params.size()) {
            const auto &param = params[i];
            ret.append(param.first);
            ret.append(" : ");
            ret.append(param.second);
            if (i < params.size() - 1) {
                ret.append(", ");
            }
            i++;
        }
        ret.append(1, ')');
        if (returnType.has_value()) {
            ret.append(" : ");
            ret.append(returnType.value());
            ret.append(1, ' ');
        }
        ret.append("{\n");
        ret.append(body.representation());
        ret.append("\n}");
        return ret;
    }

    void scope_ends() override {
        // do nothing, as we don't want to delete this value
    }

    Scope body; ///< The body of the function.

private:
    std::string name; ///< The name of the function.
    func_params params;
    std::optional<std::string> returnType;
    InterpretScope *declarationScope;
    Value *interpretReturn;
};