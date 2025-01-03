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

struct InterfaceDefinitionExtData {

    /**
     * the access specifier
     */
    AccessSpecifier specifier;

    /**
     * this is set to true when even a single implementation is detected
     */
    bool has_implementation = false;

    /**
     * is this interface deprecated
     */
    bool deprecated = false;

};

static_assert(sizeof(InterfaceDefinitionExtData) <= 8);

class InterfaceDefinition : public ExtendableMembersContainerNode {
public:

    /**
     * the parent node of the interface
     */
    ASTNode* parent_node;
    /**
     * the location of the interface
     */
    SourceLocation location;
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
     * the active user, can be retrieved by functions to see for which
     * struct is code being generated for by interface
     */
    StructDefinition* active_user = nullptr;

    /**
     * some data is stored in this struct to make it occupy less size
     */
    InterfaceDefinitionExtData data;

    /**
     * constructor
     */
    InterfaceDefinition(
            LocatedIdentifier identifier,
            ASTNode* parent_node,
            SourceLocation location,
            AccessSpecifier specifier = AccessSpecifier::Internal
    );

    /**
     * get the name of node
     */
    inline LocatedIdentifier* get_located_id() {
        return &identifier;
    }

    [[nodiscard]]
    inline AccessSpecifier specifier() const {
        return data.specifier;
    }

    inline void set_specifier_fast(AccessSpecifier specifier) {
        data.specifier = specifier;
    }

    inline bool deprecated() {
        return data.deprecated;
    }

    inline void set_deprecated(bool value) {
        data.deprecated = value;
    }

    SourceLocation encoded_location() final {
        return location;
    }

    ASTNodeKind kind() final {
        return ASTNodeKind::InterfaceDecl;
    }

    void set_parent(ASTNode* new_parent) final {
        parent_node = new_parent;
    }

    ASTNode *parent() final {
        return parent_node;
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
        data.has_implementation = true;
    }

    void register_impl(ImplDefinition* definition);

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    void declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) final;

    BaseType* create_value_type(ASTAllocator& allocator) final;

    int vtable_function_index(FunctionDeclaration* decl);

    uint64_t byte_size(bool is64Bit) final;

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
    void code_gen(Codegen &gen) final;

    /**
     * llvm type for this interface, this is void by default
     */
    llvm::Type* llvm_type(Codegen &gen) final;

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