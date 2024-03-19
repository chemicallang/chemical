// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/base/ASTNode.h"

class FunctionParam : public ASTNode {
public:
    FunctionParam(std::string  name, std::unique_ptr<BaseType> type, unsigned int index) : name(std::move(name)), type(std::move(type)), index(index) {

    }
    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }
    FunctionParam* as_parameter() override {
        return this;
    }
#ifdef COMPILER_BUILD
    llvm::Type* llvm_type(Codegen &gen) override {
        return type->llvm_type(gen);
    }
#endif
    std::string representation() const override {
        return name + " : " + type->representation();
    }
    unsigned int index;
    std::string name;
    std::unique_ptr<BaseType> type;
};


using func_params = std::vector<FunctionParam>;

class FunctionDeclaration : public ASTNode {
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
            std::unique_ptr<BaseType> returnType,
            bool isVariadic
    ) : name(std::move(name)), params(std::move(params)), returnType(std::move(returnType)), body(std::nullopt), isVariadic(isVariadic) {
        for (auto &param: this->params) {
            param.name.shrink_to_fit();
        }
    }

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

#ifdef COMPILER_BUILD
    std::vector<llvm::Type*> param_types(Codegen& gen) {
        auto size = isVariadic ? (params.size() - 1 ): params.size();
        std::vector<llvm::Type*> array(size);
        unsigned i = 0;
        while(i < size) {
            array[i] = params[i].type->llvm_type(gen);
            i++;
        }
        return array;
    }

    llvm::FunctionType* function_type(Codegen& gen) {
        if(params.empty() || (params.size() == 1 && isVariadic)) {
            return llvm::FunctionType::get(returnType->llvm_type(gen), isVariadic);
        } else {
            return llvm::FunctionType::get(returnType->llvm_type(gen), param_types(gen), isVariadic);
        }
    }

    void declare(Codegen& gen) {
        for(const auto& param : params) {
            gen.current[param.name] = (ASTNode*) &param;
        }
    }

    void undeclare(Codegen &gen) override {
        for(const auto& param : params) {
            gen.current.erase(param.name);
        }
    }

    void code_gen(Codegen& gen) override {
        if(body.has_value()) {
            declare(gen);
            gen.create_function(name, function_type(gen));
            body->code_gen(gen);
            undeclare(gen);
        } else {
            gen.declare_function(name, function_type(gen));
        }
    }
#endif

    void interpret(InterpretScope &scope) override {
        scope.global->nodes[name] = this;
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
            child.global->values[params[i].name] = call_params[i]->copy();
            i++;
        }
        body.value().interpret(child);
        return interpretReturn;
    }

    // called by the return statement
    void set_return(Value *value) {
        interpretReturn = value;
    }

    FunctionDeclaration * as_function() override {
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
            ret.append(param.representation());
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
        ret.append(" : ");
        ret.append(returnType->representation());
        ret.append(1, ' ');
        if(body.has_value()) {
            ret.append("{\n");
            ret.append(body.value().representation());
            ret.append("\n}");
        }
        return ret;
    }

    std::optional<Scope> body; ///< The body of the function.

private:
    std::string name; ///< The name of the function.
    func_params params;
    std::unique_ptr<BaseType> returnType;
    InterpretScope *declarationScope;
    Value *interpretReturn;
    // if the function is variadic, the last type in params is the type given to the variadic parameter
    bool isVariadic;
};