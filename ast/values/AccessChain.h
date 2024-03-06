// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 29/02/2024.
//

#pragma once

#include <memory>
#include "ast/base/Value.h"

class AccessChain : public Value {

public:

    AccessChain(std::vector<std::unique_ptr<Value>> values) : values(std::move(values)) {

    }

    void interpret(InterpretScope &scope) override {
        for(const auto& value:values) {
            value->interpret(scope);
        }
    }

    void set_identifier_value(InterpretScope& scope, Value *value) override {
        values[0]->set_identifier_value(scope, value);
    }

    void set_identifier_value(InterpretScope& scope, Value *value, Operation op) override {
        values[0]->set_identifier_value(scope, value, op);
    }

    Value* travel(InterpretScope& scopeVars) override {
        Value* scopeVariable = values[0]->find_in_parent(scopeVars);
        if(scopeVariable == nullptr) {
            std::cerr << "Not found in parent";
            return nullptr;
        }
        if(values.size() > 1) {
            auto i = 1;
            while(i < values.size()) {
                auto child = scopeVariable->getChild(values[i]->getScopeIdentifier());
                if(child != nullptr) {
                    scopeVariable = child;
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