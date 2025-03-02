// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ExtendableMembersContainerNode.h"
#include "ast/types/LinkedType.h"

struct VariantDeclAttributes {

    AccessSpecifier specifier;

    bool is_comptime;

    bool is_compiler_decl;

    bool deprecated;

    bool anonymous;

};

class VariantDefinition : public ExtendableMembersContainerNode {
public:

    VariantDeclAttributes attrs;
    LinkedType ref_type;

#ifdef COMPILER_BUILD
    /**
     * the key here is the generic iteration, where as the value corresponds to
     * a llvm struct type, we create a struct type once, and then cache it
     */
    std::unordered_map<int16_t, llvm::StructType*> llvm_struct_types;
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
        ref_type(name_view(), this, location), attrs(specifier, false, false, false, false) {
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


        bool is_generic() {
        return !generic_params.empty();
    }

    bool is_exported_fast() {
        return specifier() == AccessSpecifier::Public;
    }

    void declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) final;

    void link_signature(SymbolResolver &linker) override;

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

    BaseType* known_type() final;

    ASTNode* child(const chem::string_view &child_name) final;

    uint64_t byte_size(bool is64Bit) final;

    BaseType* create_value_type(ASTAllocator& allocator) final;

//    hybrid_ptr<BaseType> get_value_type() final;
    /**
     * a variant call notifies a definition, during symbol resolution that it exists
     * when this happens, generics are checked, proper types are registered in generic
     * @return iteration that corresponds to this call
     */
    int16_t register_call(SymbolResolver& resolver, FunctionCall* call, BaseType* expected_type);

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