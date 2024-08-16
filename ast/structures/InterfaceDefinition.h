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
#ifdef COMPILER_BUILD
    tsl::ordered_map<StructDefinition*, std::unordered_map<FunctionDeclaration*, llvm::Function*>> users;
#else
    tsl::ordered_map<StructDefinition*, bool> users;
#endif
    /**
     * this is set to true when even a single implementation is detected
     */
    bool has_implementation = false;
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
#ifdef COMPILER_BUILD
        users[definition] = {};
#else
        users[definition] = true;
#endif
        has_implementation = true;
    }

    void register_impl(ImplDefinition* definition);

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    void declare_top_level(SymbolResolver &linker) override;

    std::unique_ptr<BaseType> create_value_type() override;

    hybrid_ptr<BaseType> get_value_type() override;

    int vtable_function_index(FunctionDeclaration* decl);

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override;

    llvm::Type* llvm_type(Codegen &gen) override;

    void llvm_vtable_type(Codegen& gen, std::vector<llvm::Type*>& struct_types);

    llvm::Type* llvm_vtable_type(Codegen& gen);

    void llvm_build_vtable(Codegen& gen, StructDefinition* for_struct, std::vector<llvm::Constant*>& llvm_pointers);

#endif

};