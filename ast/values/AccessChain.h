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

    void interpret(InterpretScope &scope) const override {
        for(const auto& value:values) {
            value->interpret(scope);
        }
    }

    void set_in_parent(scope_vars vars, InterpretValue *value) override {
        values[0]->set_in_parent(vars, value);
    }

    InterpretValue* travel(std::unordered_map<std::string, InterpretValue*>& scopeVars) override {
        InterpretValue* scopeVariable = values[0]->find_in_parent(scopeVars);
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