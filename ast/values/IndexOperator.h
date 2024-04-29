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

    IndexOperator(std::string identifier, std::vector<std::unique_ptr<Value>> indexes) : identifier(std::move(identifier)),
                                                                          values(std::move(indexes)) {

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

    Value *find_in(InterpretScope &scope, Value *parent) override;

#ifdef COMPILER_BUILD

    // TODO isInBounds optimization, when we know that index is in bounds
    llvm::Value *elem_pointer(Codegen &gen, ASTNode *arr);

    llvm::Value *llvm_pointer(Codegen &gen) override;

    llvm::Value *llvm_value(Codegen &gen) override;

    llvm::Type *llvm_type(Codegen &gen) override;

    bool add_member_index(Codegen &gen, ASTNode *parent, std::vector<llvm::Value *> &indexes) override;

#endif

    std::unique_ptr<BaseType> create_type() const override;

    std::string representation() const override {
        std::string rep(identifier);
        rep.append(1, '[');
        for(auto& value : values) {
            rep.append(value->representation());
            rep.append(1, ',');
        }
        rep.append(1, ']');
        return rep;
    }

    ASTNode *linked;
    std::string identifier;
    std::vector<std::unique_ptr<Value>> values;

};