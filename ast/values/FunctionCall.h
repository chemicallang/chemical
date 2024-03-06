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

    void interpret(InterpretScope &scope) override {
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
            std::cerr << "Not implemented Function Call " << name;
        }
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