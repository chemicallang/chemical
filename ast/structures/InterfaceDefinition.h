// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/base/ASTNode.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/base/Value.h"
#include "MembersContainer.h"
#include "ast/base/ExtendableMembersContainerNode.h"

class InterfaceDefinition : public ExtendableMembersContainerNode {
public:

    ASTNode* parent_node;
    /**
     * users are registered so we can declare functions before hand
     */
    tsl::ordered_map<StructDefinition*, bool> users;

    /**
     * @brief Construct a new InterfaceDeclaration object.
     *
     * @param name The name of the interface.
     * @param methods The methods declared in the interface.
     */
    InterfaceDefinition(
            std::string name,
            ASTNode* parent_node
    );

    void set_parent(ASTNode* new_parent) override {
        parent_node = new_parent;
    }

    ASTNode *parent() override {
        return parent_node;
    }

    std::string ns_node_identifier() override {
        return name;
    }

    InterfaceDefinition *as_interface_def() override {
        return this;
    }

    void register_use(StructDefinition* definition) {
        users[definition] = true;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    void declare_top_level(SymbolResolver &linker) override;

    std::unique_ptr<BaseType> create_value_type() override;

    hybrid_ptr<BaseType> get_value_type() override;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override;

    llvm::Type* llvm_type(Codegen &gen) override;

#endif

};