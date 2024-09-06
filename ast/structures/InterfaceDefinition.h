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

    AccessSpecifier specifier;
    ASTNode* parent_node;
    CSTToken* token;
    /**
     * users are registered so we can declare functions before hand
     */
#ifdef COMPILER_BUILD
    tsl::ordered_map<StructDefinition*, std::unordered_map<FunctionDeclaration*, llvm::Function*>> users;
#else
    tsl::ordered_map<StructDefinition*, bool> users;
#endif
#ifdef COMPILER_BUILD
    /**
     * this maps structs that implement this interface with their global variable pointers
     * for the vtable generated
     */
    std::unordered_map<StructDefinition*, llvm::Value*> vtable_pointers;
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
            ASTNode* parent_node,
            CSTToken* token,
            AccessSpecifier specifier = AccessSpecifier::Internal
    );

    CSTToken *cst_token() override {
        return token;
    }

    ASTNodeKind kind() override {
        return ASTNodeKind::InterfaceDecl;
    }

    void set_parent(ASTNode* new_parent) override {
        parent_node = new_parent;
    }

    ASTNode *parent() override {
        return parent_node;
    }

    const std::string& ns_node_identifier() override {
        return name;
    }

    /**
     * direct usage of this struct is registered with this interface
     * the interfaces inherited by this interface aren't registered
     * which should be registered by calling register_use_to_inherited_interfaces
     */
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

    void declare_top_level(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) override;

    std::unique_ptr<BaseType> create_value_type() override;

    hybrid_ptr<BaseType> get_value_type() override;

    int vtable_function_index(FunctionDeclaration* decl);

#ifdef COMPILER_BUILD

    /**
     * this declaration will be generated for all the users of this interface
     */
    void code_gen_for_users(Codegen& gen, FunctionDeclaration* decl);

    /**
     * responsible for generating code for a single function in a union decl
     * read the documentation in this decl
     */
    void code_gen_function_declare(Codegen& gen, FunctionDeclaration* decl);

    /**
     * responsible for generating code for a single function in a union decl
     * read the documentation in this decl
     */
    void code_gen_function_body(Codegen& gen, FunctionDeclaration* decl);

    /**
     * generate code for this interface
     */
    void code_gen(Codegen &gen) override;

    /**
     * llvm type for this interface, this is void by default
     */
    llvm::Type* llvm_type(Codegen &gen) override;

    /**
     * get the vtable type into the given struct_types vector
     */
    void llvm_vtable_type(Codegen& gen, std::vector<llvm::Type*>& struct_types);

    /**
     * get the vtable type for this interface
     */
    llvm::Type* llvm_vtable_type(Codegen& gen);

    /**
     * build the vtable, put the cconstant function pointers into given llvm_pointer vector
     */
    void llvm_build_vtable(Codegen& gen, StructDefinition* for_struct, std::vector<llvm::Constant*>& llvm_pointers);

    /**
     * a helper function to build the vtable as a constant
     */
    llvm::Constant* llvm_build_vtable(Codegen& gen, StructDefinition* for_struct);

    /**
     * the vtable will be created as a global constant for the given struct
     * if a vtable already exists for the given struct, we just return it without creating another one
     */
    llvm::Value* llvm_global_vtable(Codegen& gen, StructDefinition* for_struct);

#endif

};