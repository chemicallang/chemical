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

    InterfaceDefinition* linked;
    StructDefinition* struct_linked;
    std::optional<std::string> struct_name; ///< The name of the struct.
    std::string interface_name;
    ASTNode* parent_node;

    /**
     * @brief Construct a new ImplDefinition object.
     *
     * @param name The name of the struct.
     * @param fields The members of the struct.
     */
    ImplDefinition(
            std::string interface_name,
            std::optional<std::string> struct_name,
            ASTNode* parent_node
    ) : struct_name(std::move(struct_name)), interface_name(std::move(interface_name)), parent_node(parent_node) {}

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