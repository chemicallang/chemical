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

    FunctionCall(std::string name, std::vector<std::unique_ptr<Value>> values) : name(std::move(name)), values(std::move(values)) {

    }

    FunctionCall(FunctionCall&& other) : values(std::move(other.values)) {

    }

    Value * evaluated_value(InterpretScope &scope) override {
        if(name == "print" || name == "println") {
            for(auto const& value: values){
                auto func = value->evaluated_value(scope);
                if(func) {
                    std::cout << func->interpret_representation();
                    if(name == "println") std::cout << std::endl;
                } else {
                    std::cerr << "[FunctionCall] Function parameter not found : " << value->representation();
                }
            }
        } else {
            auto func = scope.find(name);
            if(func.first) {
                return func.second->second->as_function()->call(values);
            } else {
                scope.error("[FunctionCall] couldn't find function with name " + name);
            }
        }
        return nullptr;
    }

    void interpret(InterpretScope &scope) override {
        auto value = evaluated_value(scope);
        if(value != nullptr && value->primitive()) {
            delete value;
        }
    }

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

};