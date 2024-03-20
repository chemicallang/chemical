// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 29/02/2024.
//

#pragma once

#include <vector>
#include <memory>
#include "ast/base/Value.h"

class FunctionCall : public Value {

public:

    FunctionCall(std::string name, std::vector<std::unique_ptr<Value>> values) : name(std::move(name)),
                                                                                 values(std::move(values)) {

    }

    FunctionCall(FunctionCall &&other) = delete;

    Value *find_in(Value *parent) override {
        return parent->child(name);
    }

    void prepare(InterpretScope &scope) {
        auto decl = scope.global->nodes.find(name);
        if (decl == scope.global->nodes.end()) {
            scope.error("(function call) couldn't find function declaration by name " + name);
        } else if (decl->second->as_function() == nullptr) {
            scope.error("(function call) declaration by name " + name + " is not a function");
        } else {
            definition = decl->second->as_function();
        }
    }

    Value *evaluated_value(InterpretScope &scope) override {
        if (name == "printf") {
            for (auto const &value: values) {
                auto func = value->evaluated_value(scope);
                if (func) {
                    std::cout << func->interpret_representation();
                } else {
                    scope.error("(function call) Function parameter not found : " + value->representation());
                }
            }
        } else {
            prepare(scope);
            if (definition != nullptr) {
                return definition->call(values);
            } else {
                scope.error("(function call) calling a function with no body, name : " + name);
            }
        }
        return nullptr;
    }

    Value *initializer_value(InterpretScope &scope) override {
        return evaluated_value(scope);
    }

    void interpret(InterpretScope &scope) override {
        auto value = evaluated_value(scope);
        if (value != nullptr && value->primitive()) {
            delete value;
        }
    }

#ifdef COMPILER_BUILD
    llvm::Value * llvm_value(Codegen &gen) override {
        auto fn = gen.module->getFunction(name);
        if(fn == nullptr) {
            gen.error("function with name " + name + " does not exist");
            return nullptr;
        }
        std::vector<llvm::Value*> args(values.size());
        for(size_t i = 0; i < values.size(); ++i) {
            args[i] = values[i]->llvm_value(gen);
            // Ensure proper type promotion for float values passed to printf
            if (fn->isVarArg() && llvm::isa<llvm::ConstantFP>(args[i]) && args[i]->getType() != llvm::Type::getDoubleTy(*gen.ctx)) {
                args[i] = gen.builder->CreateFPExt(args[i], llvm::Type::getDoubleTy(*gen.ctx));
            }
        }
        return gen.builder->CreateCall(fn, args);
    }

    void code_gen(Codegen &gen) override {
      llvm_value(gen);
    }
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