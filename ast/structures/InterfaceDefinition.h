// Copyright (c) Chemical Language Foundation 2025.

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
#include "ast/types/LinkedType.h"

struct InterfaceDefinitionAttrs {

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

    /**
     * is the interface a static interface (only one implementation allowed)
     */
    bool is_static = false;

    /**
     * this interface won't be prefixed with module name
     */
    bool is_no_mangle = false;

};

static_assert(sizeof(InterfaceDefinitionAttrs) <= 8);

struct OverridableFunctionInfo {
#ifdef COMPILER_BUILD
    llvm::Function* func_pointer;
#else
    FunctionDeclaration* func_pointer;
#endif
    // this flag is set by struct (if it comes before interface and overrides)
    // so that interface doesn't generate code for the default implementation
    bool overridden = false;
};

class InterfaceDefinition : public ExtendableMembersContainerNode {
public:

    /**
     * linked type to self
     */
    LinkedType linked_type;

    /**
     * the parent node of the interface
     */
    /**
     * users are registered so we can declare functions before hand
     */
#ifdef COMPILER_BUILD
    tsl::ordered_map<StructDefinition*, std::unordered_map<FunctionDeclaration*, OverridableFunctionInfo>> users;
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
    InterfaceDefinitionAttrs attrs;

    /**
     * constructor
     */
    InterfaceDefinition(
            LocatedIdentifier identifier,
            ASTNode* parent_node,
            SourceLocation location,
            AccessSpecifier specifier = AccessSpecifier::Internal
    ) : ExtendableMembersContainerNode(identifier, ASTNodeKind::InterfaceDecl, parent_node, location),
        attrs(specifier, false, false, false), linked_type(this) {

    }

    /**
     * get the name of node
     */
    inline LocatedIdentifier* get_located_id() {
        return &identifier;
    }

    [[nodiscard]]
    inline AccessSpecifier specifier() const {
        return attrs.specifier;
    }

    inline void set_specifier_fast(AccessSpecifier specifier) {
        attrs.specifier = specifier;
    }

    inline bool deprecated() {
        return attrs.deprecated;
    }

    inline void set_deprecated(bool value) {
        attrs.deprecated = value;
    }

    inline bool is_static() {
        return attrs.is_static;
    }

    inline void set_is_static(bool value) {
        attrs.is_static = value;
    }

    inline bool has_implementation() {
        return attrs.has_implementation;
    }

    inline bool is_no_mangle() {
        return attrs.is_no_mangle;
    }

    inline void set_no_mangle(bool no_mangle) {
        attrs.is_no_mangle = no_mangle;
    }

    InterfaceDefinition* shallow_copy(ASTAllocator& allocator) {
        const auto def = new (allocator.allocate<InterfaceDefinition>()) InterfaceDefinition(
                identifier, parent(), encoded_location(), specifier()
        );
        def->attrs = attrs;
        ExtendableMembersContainerNode::shallow_copy_into(*def, allocator);
        return def;
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
        users[definition] = false;
#endif
        attrs.has_implementation = true;
    }

    void register_impl(ImplDefinition* definition);

    int vtable_function_index(FunctionDeclaration* decl);

    uint64_t byte_size(bool is64Bit) final;

    BaseType* known_type() override {
        return &linked_type;
    }

#ifdef COMPILER_BUILD

    /**
     * this declaration will be generated for all the users of this interface
     */
    void code_gen_declare_for_users(Codegen& gen, FunctionDeclaration* decl);

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
     * sets up function pointers for users of this interface
     */
    void code_gen_declare(Codegen &gen) override;

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
     * create a vtable type for the given struct, only call this if you don't need to create
     * the vtable constant, otherwise you can call getType on the constant vtable pointer
     */
    llvm::StructType* llvm_vtable_type(Codegen& gen);

    /**
     * build the vtable, put the cconstant function pointers into given llvm_pointer vector
     */
    void llvm_build_vtable(Codegen& gen, StructDefinition* for_struct, std::vector<llvm::Constant*>& llvm_pointers);

protected:

    /**
     * a helper function to build the vtable as a constant
     */
    llvm::Constant* llvm_build_vtable(Codegen& gen, StructDefinition* for_struct, llvm::StructType* vtable_type);

    /**
     * a helper function to build the vtable as a constant
     */
    inline llvm::Constant* llvm_build_vtable(Codegen& gen, StructDefinition* for_struct) {
        return llvm_build_vtable(gen, for_struct, llvm_vtable_type(gen));
    }

public:

    /**
     * the vtable will be created as a global constant for the given struct
     * if a vtable already exists for the given struct, we just return it without creating another one
     */
    llvm::Value* create_global_vtable(Codegen& gen, StructDefinition* for_struct, bool declare_only);

    /**
     * the vtable for primitive impl
     */
    llvm::Value* create_global_vtable(Codegen& gen, ImplDefinition* implDef, BaseType* implType, bool declare_only);

    /**
     * the vtable will be created as a global constant for the given struct
     * if a vtable already exists for the given struct, we just return it without creating another one
     */
    llvm::Value* llvm_global_vtable(Codegen& gen, StructDefinition* for_struct) {
        auto found = vtable_pointers.find(for_struct);
        return found != vtable_pointers.end() ? found->second : create_global_vtable(gen, for_struct, false);
    }

    /**
     * externally declares the
     */
    void code_gen_external_declare(Codegen &gen) override;

#endif

};