// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 29/02/2024.
//

#pragma once

#include <memory>
#include "ast/base/Value.h"

class AccessChain : public ASTNode, public Value {

public:

    AccessChain(std::vector<std::unique_ptr<Value>> values) : values(std::move(values)) {

    }

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

    void interpret(InterpretScope &scope) override {
        for(const auto& value:values) {
            value->interpret(scope);
        }
    }

    void code_gen(Codegen &gen) override {
        for(const auto& value : values) {
            value->code_gen(gen);
        }
    }

    llvm::Value * llvm_value(Codegen &gen) override {
        return values[values.size() - 1]->llvm_value(gen);
//        gen.error("Unimplemented accessing complete access chain as llvm value");
//        return nullptr;
    }

    llvm::Value * llvm_pointer(Codegen &gen) override {
        return values[values.size() - 1]->llvm_pointer(gen);
//        gen.error("Unimplemented accessing complete access chain as llvm pointer");
//        return nullptr;
    }

    void set_identifier_value(InterpretScope& scope, Value *value) override {
        values[0]->set_identifier_value(scope, value);
    }

    void set_identifier_value(InterpretScope& scope, Value *value, Operation op) override {
        values[0]->set_identifier_value(scope, value, op);
    }

    std::string interpret_representation() const override {
        return "[AccessChainInterpretRepresentation]";
    }

    Value * evaluated_value(InterpretScope &scope) override {
        Value* scopeVariable = values[0]->evaluated_value(scope);
        if(scopeVariable == nullptr) {
            scope.printAllValues();
            std::cerr << "Not found in parent : " << values[0]->representation();
            return nullptr;
        }
        if(values.size() > 1) {
            auto i = 1;
            while(i < values.size()) {
                auto child = values[i]->find_in(scopeVariable);
                if(child != nullptr) {
                    scopeVariable = child;
                } else {
                    std::cerr << "in access chain" << representation() << " not found at " << std::to_string(i) << " : " << values[i]->representation();
                }
                i++;
            }
        }
        return scopeVariable;
    }

    std::string representation() const override {
        std::string rep;
        int i = 0;
        while (i < values.size()) {
            rep.append(values[i]->representation());
            if (i != values.size() - 1) {
                rep.append(1, '.');
            }
            i++;
        }
        return rep;
    }

    std::vector<std::unique_ptr<Value>> values;

};