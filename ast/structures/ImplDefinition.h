// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/base/Value.h"
#include "ast/structures/InterfaceDefinition.h"

class ImplDefinition : public MembersContainer {
public:

    std::unique_ptr<BaseType> interface_type;
    std::unique_ptr<BaseType> struct_type;
    ASTNode* parent_node;

    /**
     * constructor
     */
    ImplDefinition(
            std::unique_ptr<BaseType> interface_type,
            std::unique_ptr<BaseType> struct_type,
            ASTNode* parent_node
    );

    /**
     * constructor
     */
    ImplDefinition(
            ASTNode* parent_node
    );

    void set_parent(ASTNode* new_parent) override {
        parent_node = new_parent;
    }

    ASTNode *parent() override {
        return parent_node;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    ImplDefinition *as_impl_def() override {
        return this;
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override;

#endif

    void declare_and_link(SymbolResolver &linker) override;

};