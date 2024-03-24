// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 29/02/2024.
//

#pragma once

#include <memory>
#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"

class AccessChain : public ASTNode, public Value {

public:

    AccessChain(std::vector<std::unique_ptr<Value>> values) : values(std::move(values)) {

    }

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

    bool primitive() override {
        return false;
    }

    bool reference() override {
        return true;
    }

    void interpret(InterpretScope &scope) override {
        auto v = evaluated_value(scope);
        if(v != nullptr && v->primitive()) {
            delete v;
        }
    }

#ifdef COMPILER_BUILD
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
#endif

    Value *parent(InterpretScope &scope) {
        Value *current = values[0].get();
        unsigned i = 1;
        while (i < (values.size() - 1)) {
            current = values[i]->find_in(scope, current);
            if (current == nullptr) {
                scope.error(
                        "(access chain) " + representation() + " child " + values[i]->representation() + " not found");
                return nullptr;
            }
            i++;
        }
        return current;
    }

    inline Value* parent_value(InterpretScope &scope) {
#ifdef DEBUG
        auto p = parent(scope);
        if(p == nullptr) {
            scope.error("parent is nullptr in access cain " + representation());
        } else if(p->evaluated_value(scope) == nullptr) {
            scope.error("evaluated value of parent is nullptr in access chain " + representation() + " pointer " + p->representation());
        }
#endif
        return parent(scope)->evaluated_value(scope);
    }

    void set_identifier_value(InterpretScope &scope, Value *rawValue, Operation op) override {
        if (values.size() <= 1) {
            values[0]->set_identifier_value(scope, rawValue, op);
        } else {
            values[values.size() - 1]->set_value_in(scope, parent_value(scope), rawValue->assignment_value(scope), op);
        }
    }

    std::string interpret_representation() const override {
        return "[AccessChainInterpretRepresentation]";
    }

    Value *evaluated_value(InterpretScope &scope) override {
        if (values.size() <= 1) {
            return values[0]->evaluated_value(scope);
        } else {
            return values[values.size() - 1]->find_in(scope, parent_value(scope));
        }
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