#include <utility>

// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

class AssignStatement : public ASTNode {
public:

    /**
     * @brief Construct a new AssignStatement object.
     *
     * @param identifier The identifier being assigned.
     * @param value The value being assigned to the identifier.
     */
    AssignStatement(
            std::unique_ptr<AccessChain> lhs,
            std::unique_ptr<Value> value,
            Operation assOp
    ) : lhs(std::move(lhs)), value(std::move(value)), assOp(assOp) {}

    void code_gen(Codegen &gen) override {
        if(assOp == Operation::Assignment) {
            gen.builder->CreateStore(value->llvm_value(gen), lhs->llvm_pointer(gen));
        } else {
            auto loaded = lhs->llvm_value(gen);
            auto operated = gen.operate(assOp, loaded, value->llvm_value(gen));
            gen.builder->CreateStore(operated, lhs->llvm_pointer(gen));
        }
    }

    void interpret(InterpretScope& scope) override {
        Value* next;
        if(value->primitive()) {
            next = value->copy();
        } else {
            next = value->evaluated_value(scope);
        }
        if(assOp == Operation::Assignment) {
            lhs->set_identifier_value(scope, next);
        } else {
            lhs->set_identifier_value(scope, next, assOp);
        }
    }

    std::string representation() const override {
        std::string rep;
        rep.append(lhs->representation());
        if(assOp != Operation::Assignment) {
            rep.append(" " + to_string(assOp) +"= ");
        } else {
            rep.append(" = ");
        }
        rep.append(value->representation());
        return rep;
    }

private:
    std::unique_ptr<AccessChain> lhs; ///< The identifier being assigned.
    std::unique_ptr<Value> value; ///< The value being assigned to the identifier.
    Operation assOp;
};