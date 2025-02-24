// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"
#include "ast/base/AnnotableNode.h"
#include <optional>
#include "ast/base/BaseType.h"
#include "ast/base/AccessSpecifier.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/base/LocatedIdentifier.h"

struct VarInitAttributes {

    /**
     * the access specifier of the declaration
     */
    AccessSpecifier specifier;

    /**
     * is var init comptime
     */
    bool is_comptime;

    /**
     * compiler declarations are present inside the compiler, no need to import
     */
    bool is_compiler_decl;

    /**
     * has moved is used to indicate that an object at this location has moved
     * destructor is not called on moved objects, once moved, any attempt to access
     * this variable causes an error
     */
    bool has_moved;
    /**
     * has moved is used to track that var init statement has a assignment to it
     * if it has, during symbol resolution the assignment statement notifies this
     * var init statement
     */
    bool has_assignment;

    /**
     * this tells whether wrote const or var when creating the variable
     */
    bool is_const;

    /**
     * true when user writes var& or const& to take a reference
     */
    bool is_reference;

    /**
     * is var init deprecated
     */
    bool deprecated;

};

class VarInitStatement : public ASTNode {
public:

    // TODO do not store the decl_scope here
    InterpretScope *decl_scope = nullptr;
    LocatedIdentifier located_id; ///< The identifier being initialized.
    BaseType* type;
    Value* value; ///< The value being assigned to the identifier.
    ASTNode* parent_node;
#ifdef COMPILER_BUILD
    llvm::Value *llvm_ptr;
#endif

    /**
     * extra data for the var init
     */
    VarInitAttributes attrs;

    /**
     * constructor
     */
    VarInitStatement(
            bool is_const,
            bool is_reference,
            LocatedIdentifier identifier,
            BaseType* type,
            Value* value,
            ASTNode* parent_node,
            SourceLocation location,
            AccessSpecifier specifier = AccessSpecifier::Internal
    ) : ASTNode(ASTNodeKind::VarInitStmt, location), attrs(specifier, false, false, false, false, is_const, is_reference, false), located_id(identifier),
        type(type), value(value), parent_node(parent_node) {

    }

    /**
     * get the name of node
     */
    inline LocatedIdentifier* get_located_id() {
        return &located_id;
    }

    /**
     * get the name view
     */
    inline const chem::string_view& name_view() {
        return located_id.identifier;
    }

    /**
     * get the name / identifier of the declaration
     */
    inline const chem::string_view& id_view() {
        return located_id.identifier;
    }

    /**
     * get the name as a string
     */
    inline std::string name_str() {
        return id_view().str();
    }

    inline bool deprecated() {
        return attrs.deprecated;
    }

    inline void set_deprecated(bool value) {
        attrs.deprecated = value;
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

    /**
     * get the access specifier
     */
    inline AccessSpecifier specifier() const {
        return attrs.specifier;
    }

    /**
     * set's the specifier of this decl fast
     */
    inline void set_specifier(AccessSpecifier specifier) {
        attrs.specifier = specifier;
    }

    /**
     * check is this declaration const
     */
    inline bool is_const() {
        return attrs.is_const;
    }

    /**
     * check is this a reference like const& or var&
     */
    inline bool is_reference() {
        return attrs.is_reference;
    }

    /**
     * check this variable has been moved
     */
    inline bool get_has_moved() const {
        return attrs.has_moved;
    }

    /**
     * call it when this variable has been moved
     */
    inline void moved() {
        attrs.has_moved = true;
    }

    /**
     * call it when this variable should be unmoved
     */
    inline void unmove() {
        attrs.has_moved = false;
    }

    /**
     * get has assignment
     */
    inline bool get_has_assignment() {
        return attrs.has_assignment;
    }

    /**
     * assignment can be set to true
     */
    inline void set_has_assignment() {
        attrs.has_assignment = true;
    }


    void set_parent(ASTNode* new_parent) final {
        parent_node = new_parent;
    }

    ASTNode *parent() final {
        return parent_node;
    }

    Value* holding_value() final {
        return value;
    }

    BaseType* known_type() final;

    bool is_top_level();

    BaseType* type_ptr_fast() {
        return type;
    }

#ifdef COMPILER_BUILD

    inline void check_has_type(Codegen &gen);

    llvm::Value *llvm_pointer(Codegen &gen) final;

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) final;

    llvm::Value *llvm_load(Codegen &gen) final;

    bool add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) final;

    void code_gen_global_var(Codegen &gen, bool initialize);

    void code_gen(Codegen &gen) final;

    void code_gen_destruct(Codegen &gen, Value* returnValue) final;

    void code_gen_external_declare(Codegen &gen) final;

#endif

    void runtime_name_no_parent(std::ostream &stream) final {
        stream << id_view();
    }

    inline std::string runtime_name_fast() {
        return parent_node ? runtime_name_str() : name_str();
    }

    ASTNode *child(const chem::string_view &name) final;

    void declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) final;

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

    void interpret(InterpretScope &scope) final;

    BaseType* create_value_type(ASTAllocator& allocator) final;

//    hybrid_ptr<BaseType> get_value_type() final;

    /**
     * called by assignment to assign a new value in the scope that this variable was declared
     */
    void declare(Value *new_value);

    [[nodiscard]]
    BaseTypeKind type_kind() const final;

};