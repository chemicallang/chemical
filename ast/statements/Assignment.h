#include <utility>

// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"
#include "ast/values/AccessChain.h"
#include "ast/utils/Operation.h"

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

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) override {
        if(assOp == Operation::Assignment) {
            gen.builder->CreateStore(value->llvm_value(gen), lhs->llvm_pointer(gen));
        } else {
            auto loaded = lhs->llvm_value(gen);
            auto operated = gen.operate(assOp, loaded, value->llvm_value(gen));
            gen.builder->CreateStore(operated, lhs->llvm_pointer(gen));
        }
    }
#endif

    void interpret(InterpretScope& scope) override {
        lhs->set_identifier_value(scope, value->initializer_value(scope), assOp);
    }

    void interpret_scope_ends(InterpretScope &scope) override {
        // when the var initializer ast node or the holder ast node goes out of scope
        // the newly created value due to function call initializer_value will be destroyed
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
    std::unique_ptr<AccessChain> lhs;
    std::unique_ptr<Value> value;
    InterfaceDefinition* definition;
    Operation assOp;
};