// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 29/02/2024.
//

#pragma once

#include <memory>
#include <utility>
#include "ast/base/Value.h"

class IndexOperator : public Value {
public:

    IndexOperator(std::string identifier, std::unique_ptr<Value> value) : identifier(std::move(identifier)),
                                                                          value(std::move(value)) {

    }

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

    void link(SymbolResolver &linker) override;

    ASTNode *linked_node() override;

    ASTNode *find_link_in_parent(ASTNode *parent) override;

    bool primitive() override {
        return false;
    }

    Value *find_in(InterpretScope &scope, Value *parent) override {
#ifdef DEBUG
        try {
            return parent->index(scope, value->evaluated_value(scope)->as_int());
        } catch (...) {
            std::cerr << "[InterpretError] index operator only support's integer indexes at the moment";
        }
#endif
        parent->index(scope, value->evaluated_value(scope)->as_int());
        return nullptr;
    }

#ifdef COMPILER_BUILD

    // TODO isInBounds optimization, when we know that index is in bounds
    llvm::Value* elem_pointer(Codegen& gen, ASTNode* arr);

    llvm::Value * llvm_pointer(Codegen &gen) override;

    llvm::Value * llvm_value(Codegen &gen) override;

    bool add_member_index(Codegen& gen, ASTNode* parent, std::vector<llvm::Value *> &indexes) override;

#endif

    std::unique_ptr<BaseType> create_type() const override;

    std::string representation() const override {
        std::string rep(identifier);
        rep.append(1, '[');
        rep.append(value->representation());
        rep.append(1, ']');
        return rep;
    }

    ASTNode *linked;
    std::string identifier;
    std::unique_ptr<Value> value;

};