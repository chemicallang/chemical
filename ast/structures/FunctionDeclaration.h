// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/values/VariableIdentifier.h"

using func_params = std::vector<std::pair<std::string, std::string>>;

class FunctionDeclaration : public ASTNode, public Value {
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
            std::optional<std::string> returnType,
            bool isVariadic
    ) : name(std::move(name)), params(std::move(params)), returnType(std::move(returnType)), body(std::nullopt), isVariadic(isVariadic) {
        for (auto &param: this->params) {
            param.first.shrink_to_fit();
            param.second.shrink_to_fit();
        }
    }

    llvm::Type* llvm_type(Codegen& gen, const std::optional<std::string>& type) {
        if(type.has_value()) {
            if (type.value() == "int") {
                return gen.builder->getInt32Ty();
            } else if (type.value() == "string") {
                return gen.builder->getInt8PtrTy();
            } else {
                gen.error("UNKNOWN FUNCTION RETURN TYPE : " + type.value());
                return nullptr;
            }
        } else {
            return gen.builder->getVoidTy();
        }
    }

    std::vector<llvm::Type*> param_types(Codegen& gen) {
        auto size = isVariadic ? (params.size() - 1 ): params.size();
        std::vector<llvm::Type*> array(size);
        unsigned i = 0;
        while(i < size) {
            array[i] = llvm_type(gen, params[i].second);
            i++;
        }
        return array;
    }

    llvm::FunctionType* function_type(Codegen& gen) {
        if(params.empty() || (params.size() == 1 && isVariadic)) {
            return llvm::FunctionType::get(llvm_type(gen, returnType), isVariadic);
        } else {
            return llvm::FunctionType::get(llvm_type(gen, returnType), param_types(gen), isVariadic);
        }
    }

    void code_gen(Codegen& gen) override {
        if(body.has_value()) {
            gen.create_function(name, function_type(gen));
            body->code_gen(gen);
        } else {
            gen.declare_function(name, function_type(gen));
        }
    }

    void interpret(InterpretScope &scope) override {
        scope.values[name] = this;
        declarationScope = &scope;
    }

    Value *call(std::vector<std::unique_ptr<Value>> &call_params) {
        InterpretScope child(declarationScope, declarationScope->global, &body.value(), this);
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
        body.value().interpret(child);
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
            } else {
                if(isVariadic) {
                    ret.append("...");
                }
            }
            i++;
        }
        ret.append(1, ')');
        if (returnType.has_value()) {
            ret.append(" : ");
            ret.append(returnType.value());
            ret.append(1, ' ');
        }
        if(body.has_value()) {
            ret.append("{\n");
            ret.append(body.value().representation());
            ret.append("\n}");
        }
        return ret;
    }

    void scope_ends() override {
        // do nothing, as we don't want to delete this value
    }

    std::optional<Scope> body; ///< The body of the function.

private:
    std::string name; ///< The name of the function.
    func_params params;
    std::optional<std::string> returnType;
    InterpretScope *declarationScope;
    Value *interpretReturn;
    // if the function is variadic, the last type in params is the type given to the variadic parameter
    bool isVariadic;
};