// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/base/ASTNode.h"
#include "MembersContainer.h"
#include <optional>
#include <map>
#include "ast/base/ExtendableMembersContainerNode.h"
#include "ast/types/StructType.h"
#include "ast/types/LinkedType.h"

struct StructDeclAttributes {

    /**
     * the access specifier for the declaration
     */
    AccessSpecifier specifier;

    /**
     * is the struct and it's functions comptime
     */
    bool is_comptime = false;

    /**
     * is a compiler declaration (present inside the compiler, no need to import)
     */
    bool is_compiler_decl = false;

    /**
     * is direct initialization
     * constructor or de constructor allow functions to be called automatically
     */
    bool is_direct_init = false;

    /**
     * is this struct deprecated
     */
    bool deprecated = false;

    /**
     * if struct shouldn't be initialized (user asked)
     */
    bool no_init = false;

    /**
     * struct is marked to allow use after the move
     */
    bool use_after_move = false;

    /**
     * struct marked anonymous to keep it anonymous in generated code
     */
    bool anonymous = false;

    /**
     * if abstract, cannot be instantiated, because it doesn't implement all methods of interface
     * for which a struct that inherits this, will provide the implementation
     */
    bool is_abstract = false;

    /**
     * when marked with copy annotation, this struct is easy to copy, because it will be shallow
     * copies everywhere
     */
    bool is_copy = false;

    /**
     * is no mangle means, module name won't be added to this struct
     */
    bool is_no_mangle = false;

};

class StructDefinition : public ExtendableMembersContainerNode {
private:
    StructDeclAttributes attrs;

public:
    LinkedType linked_type;

#ifdef COMPILER_BUILD
    /**
     * the key here is the generic iteration, where as the value corresponds to
     * a llvm struct type, we create a struct type once, and then cache it
     */
    llvm::StructType* llvm_struct_type = nullptr;
#endif

    /**
     * @brief Construct a new StructDeclaration object.
     *
     * @param name The name of the struct.
     * @param fields The members of the struct.
     */
    StructDefinition(
            LocatedIdentifier identifier,
            ASTNode* parent_node,
            SourceLocation location,
            AccessSpecifier specifier = AccessSpecifier::Internal
    ) : ExtendableMembersContainerNode(identifier, ASTNodeKind::StructDecl, parent_node, location),
        attrs(specifier, false, false, false, false, false, false, false, false, false),
        linked_type(this, location) {

    }

    /**
     * get the name of node
     */
    inline LocatedIdentifier* get_located_id() {
        return &identifier;
    }

    inline AccessSpecifier specifier() {
        return attrs.specifier;
    }

    inline void set_specifier(AccessSpecifier specifier) {
        attrs.specifier = specifier;
    }

    inline bool is_comptime() {
        return attrs.is_comptime;
    }

    inline void set_comptime(bool value) {
        attrs.is_comptime = value;
    }

    inline bool is_compiler_decl() {
        return attrs.is_compiler_decl;
    }

    inline void set_compiler_decl(bool value) {
        attrs.is_comptime = value;
        attrs.is_compiler_decl = value;
    }

    inline bool is_direct_init() {
        return attrs.is_direct_init;
    }

    inline void set_direct_init(bool value) {
        attrs.is_direct_init = value;
    }

    inline bool is_deprecated() {
        return attrs.deprecated;
    }

    inline void set_deprecated(bool value) {
        attrs.deprecated = value;
    }

    inline bool is_no_init() {
        return attrs.no_init;
    };

    inline void set_no_init(bool value) {
        attrs.no_init = value;
    }

    inline bool is_use_after_move() {
        return attrs.use_after_move;
    };

    inline void set_use_after_move(bool value) {
        attrs.use_after_move = value;
    }

    inline bool is_anonymous() {
        return attrs.anonymous;
    };

    inline void set_anonymous(bool value) {
        attrs.anonymous = value;
    }

    inline bool is_abstract() {
        return attrs.is_abstract;
    }

    inline void set_abstract(bool value) {
        attrs.is_abstract = value;
    }

    inline bool is_shallow_copyable() {
        return attrs.is_copy;
    }

    inline void set_shallow_copyable(bool value) {
        attrs.is_copy = value;
    }

    inline bool is_no_mangle() {
        return attrs.is_no_mangle;
    }

    inline void set_no_mangle(bool no_mangle) {
        attrs.is_no_mangle = no_mangle;
    }

    inline bool has_destructor() {
        return destructor_func() != nullptr;
    }

//    int16_t get_generic_iteration() final {
//        return active_iteration;
//    }

    StructDefinition* shallow_copy(ASTAllocator& allocator) {
        const auto def = new (allocator.allocate<StructDefinition>()) StructDefinition(
                identifier, parent(), encoded_location(), specifier()
        );
        def->attrs = attrs;
        ExtendableMembersContainerNode::shallow_copy_into(*def, allocator);
        return def;
    }

    VariablesContainer *as_variables_container() final {
        return this;
    }

    bool is_exported_fast() {
        return specifier() == AccessSpecifier::Public;
    }

    void declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) final;

    void link_signature_no_gen(SymbolResolver &linker) {
        MembersContainer::link_signature(linker);
    }

    void link_signature(SymbolResolver &linker) override;

    /**
     * generates any default constructors / destructors required
     * should be called after link signature
     */
    void generate_functions(ASTAllocator& allocator, ASTDiagnoser& diagnoser);

    inline void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final {
        MembersContainer::declare_and_link(linker, node_ptr);
        register_use_to_inherited_interfaces(this);
    }

    ASTNode *child(const chem::string_view &name) final;

    BaseType* known_type() final;

    uint64_t byte_size(bool is64Bit) final {
        return total_byte_size(is64Bit);
    }

//    [[nodiscard]]
//    BaseType* copy(ASTAllocator &allocator) const final;

#ifdef COMPILER_BUILD

    llvm::Type* with_elements_type(
        Codegen &gen,
        const std::vector<llvm::Type *>& elements,
        const std::string& runtime_name
    );

    llvm::StructType* llvm_stored_type();

    void llvm_store_type(llvm::StructType* type);

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Type *llvm_param_type(Codegen &gen) final;

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) final;

    /**
     * will try to override the given function if there's an interface and it exists
     * in the inherited struct / interface, otherwise returns false
     */
    bool llvm_override(Codegen& gen, FunctionDeclaration* declaration);

    /**
     * generate code for all functions in this struct
     */
    void struct_func_gen(
        Codegen& gen,
        InstFuncRange range,
        bool declare
    );

    /**
     * this function is responsible for declaring this single function
     * that is present inside this struct, also read the docs of body
     */
    void code_gen_function_declare(Codegen& gen, FunctionDeclaration* decl);

    /**
     * this function is responsible for generating code for a single function
     * this function is not supposed to be called, because struct decl tends to
     * generate declarations for all it's functions and then bodies, so that
     * functions above can call functions declared below
     * However this is required because generic functions inside structs can have
     * uses outside the current file, the function is queued for generation for that type
     * and then function declaration calls this function (if this struct is parent)
     */
    void code_gen_function_body(Codegen& gen, FunctionDeclaration* decl);

    void code_gen(Codegen &gen, bool declare);

    void code_gen_declare(Codegen &gen) final {
        code_gen(gen, true);
    }

    void code_gen(Codegen &gen) final {
        code_gen(gen, false);
    }

    void code_gen_external_declare(Codegen &gen) final;

    void llvm_destruct(Codegen &gen, llvm::Value *allocaInst, SourceLocation location);

#endif

};