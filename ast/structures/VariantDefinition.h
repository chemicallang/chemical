// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ExtendableMembersContainerNode.h"
#include "ast/types/LinkedType.h"

struct VariantDeclAttributes {

    AccessSpecifier specifier;

    bool is_comptime = false;

    bool is_compiler_decl = false;

    bool deprecated = false;

    bool anonymous = false;

    bool is_copy = false;

};

class VariantDefinition : public ExtendableMembersContainerNode {
public:

    VariantDeclAttributes attrs;
    LinkedType ref_type;

#ifdef COMPILER_BUILD
    /**
     * the llvm struct type
     */
    llvm::StructType* llvm_struct_type = nullptr;
#endif

    /**
     * constructor
     */
    VariantDefinition(
        LocatedIdentifier identifier,
        ASTNode* parent_node,
        SourceLocation location,
        AccessSpecifier specifier = AccessSpecifier::Internal
    ) : ExtendableMembersContainerNode(identifier, ASTNodeKind::VariantDecl, parent_node, location),
        ref_type(this, location), attrs(specifier, false, false, false, false) {
    }

    /**
     * get the name of node
     */
    inline LocatedIdentifier* get_located_id() {
        return &identifier;
    }


    AccessSpecifier specifier() {
        return attrs.specifier;
    }

    void set_specifier(AccessSpecifier specifier) {
        attrs.specifier = specifier;
    }

    inline bool is_comptime() {
        return attrs.is_comptime;
    }

    inline void set_is_comptime(bool value) {
        attrs.is_comptime = value;
    }

    inline bool is_compiler_decl() {
        return attrs.is_compiler_decl;
    }

    inline void set_compiler_decl(bool value) {
        attrs.is_comptime = value;
        attrs.is_compiler_decl = value;
    }

    inline bool is_deprecated() {
        return attrs.deprecated;
    }

    inline void set_deprecated(bool value) {
        attrs.deprecated = value;
    }

    inline bool is_anonymous() {
        return attrs.anonymous;
    }

    inline void set_anonymous(bool value) {
        attrs.anonymous = value;
    }

    inline bool is_shallow_copyable() {
        return attrs.is_copy;
    }

    inline void set_shallow_copyable(bool value) {
        attrs.is_copy = value;
    }

    bool is_exported_fast() {
        return specifier() == AccessSpecifier::Public;
    }

    VariantDefinition* shallow_copy(ASTAllocator& allocator) {
        const auto def = new (allocator.allocate<VariantDefinition>()) VariantDefinition(
                identifier, parent(), encoded_location(), specifier()
        );
        def->attrs = attrs;
        ExtendableMembersContainerNode::shallow_copy_into(*def, allocator);
        return def;
    }

    void generate_functions(ASTAllocator& allocator, ASTDiagnoser& diagnoser);

    void declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) final;

    inline void link_signature_no_gen(SymbolResolver &linker) {
        MembersContainer::link_signature(linker);
    }

    void link_signature(SymbolResolver &linker) override;

    inline void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final {
        MembersContainer::declare_and_link(linker, node_ptr);
    }

    BaseType* known_type() final;

    ASTNode* child(const chem::string_view &child_name) final;

    uint64_t byte_size(bool is64Bit) final;

    /**
     * check if it includes any member who has a struct, that requires a destructor
     */
    bool requires_destructor();

#ifdef COMPILER_BUILD

    static llvm::StructType* llvm_type_with_member(Codegen& gen, VariantMember* member, bool anonymous = true);

    llvm::Type* llvm_type(Codegen &gen) final;

    llvm::Type *llvm_type(Codegen &gen, int16_t iteration);

    llvm::Type* llvm_param_type(Codegen &gen) final;

    llvm::Type* llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) final;

    /**
     * responsible for generating code for a single function in a union decl
     * read the documentation in this decl
     */
    void code_gen_function_declare(Codegen &gen, FunctionDeclaration* decl);

    /**
     * responsible for generating code for a single function in a union decl
     * read the documentation in this decl
     */
    void code_gen_function_body(Codegen &gen, FunctionDeclaration* decl);

    void code_gen_once(Codegen &gen, bool declare);

    void code_gen(Codegen &gen, bool declare);

    void code_gen_declare(Codegen &gen) final {
        code_gen(gen, true);
    }

    void code_gen(Codegen &gen) final {
        code_gen(gen, false);
    }

    void code_gen_external_declare(Codegen &gen) final;

    void llvm_destruct(Codegen &gen, llvm::Value *allocaInst, SourceLocation location);

    bool add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) final;

#endif

};