// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"
#include "ast/base/AnnotableNode.h"
#include <optional>
#include "ast/base/TypeLoc.h"
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
    bool is_comptime = false;

    /**
     * compiler declarations are present inside the compiler, no need to import
     */
    bool is_compiler_decl = false;

    /**
     * this means that variable has atleast one move
     */
    bool has_move = false;

    /**
     * has moved is used to track that var init statement has a assignment to it
     * if it has, during symbol resolution the assignment statement notifies this
     * var init statement
     */
    bool has_assignment = false;

    /**
     * this tells whether wrote const or var when creating the variable
     */
    bool is_const = false;

    /**
     * true when user writes var& or const& to take a reference
     */
    bool is_reference = false;

    /**
     * is var init deprecated
     */
    bool deprecated = false;

    /**
     * do not mangle the variable
     */
    bool is_no_mangle = false;

    /**
     * we could resolve the type and value properly
     */
    bool signature_resolved = true;

    /**
     * whether the user gave type
     */
    bool user_gave_type = false;

};

class VarInitStatement : public ASTNode {
public:

    // TODO do not store the decl_scope here
    InterpretScope *decl_scope = nullptr;
    LocatedIdentifier located_id; ///< The identifier being initialized.
    TypeLoc type;
    Value* value; ///< The value being assigned to the identifier.

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
    constexpr VarInitStatement(
            bool is_const,
            bool is_reference,
            LocatedIdentifier identifier,
            TypeLoc type,
            Value* value,
            ASTNode* parent_node,
            SourceLocation location,
            AccessSpecifier specifier = AccessSpecifier::Internal
    ) : ASTNode(ASTNodeKind::VarInitStmt, parent_node, location),
        attrs(specifier, false, false, false, false, is_const, is_reference, false, false, true, type != nullptr),
        located_id(identifier), type(type), value(value) {

    }

    VarInitStatement* copy(ASTAllocator &allocator) override {
        const auto stmt = new (allocator.allocate<VarInitStatement>()) VarInitStatement(
            false, false,
            located_id,
            type ? type.copy(allocator) : nullptr,
            value ? value->copy(allocator) : nullptr,
            parent(),
            encoded_location(),
            AccessSpecifier::Internal
        );
        stmt->attrs = attrs;
        return stmt;
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
    inline bool get_has_move() const {
        return attrs.has_move;
    }

    /**
     * call it when this variable has been moved
     */
    inline void set_has_move(bool value) {
        attrs.has_move = value;
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

    inline bool is_no_mangle() {
        return attrs.is_no_mangle;
    }

    inline void set_no_mangle(bool no_mangle) {
        attrs.is_no_mangle = no_mangle;
    }

    Value* holding_value() final {
        return value;
    }

    BaseType* known_type() final;

    BaseType* known_type_or_err() {
        const auto k = known_type();
#ifdef DEBUG
        if(!k) {
            throw std::runtime_error("known_type not found");
        }
#endif
        return k;
    }

    BaseType* type_ptr_fast() {
        return type;
    }

    BaseType* get_or_create_type(ASTAllocator& allocator);

#ifdef COMPILER_BUILD

    inline void check_has_type(Codegen &gen);

    /**
     * this just performs implicit cast for integers on values
     */
    llvm::Value* initializer_value(Codegen &gen);

    llvm::Value *llvm_pointer(Codegen &gen) final;

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) final;

    llvm::Value *llvm_load(Codegen& gen, SourceLocation location) final;

    bool add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) final;

    void code_gen_global_var(Codegen &gen, bool initialize);

    void code_gen(Codegen &gen) final;

    void code_gen_external_declare(Codegen &gen) final;

    void put_destructible(Codegen& gen);

#endif

    ASTNode *child(const chem::string_view &name) final;

    void link_signature(SymbolResolver &linker) override;

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

    inline BaseType* known_type_SymRes(ASTAllocator& allocator) {
        return get_or_create_type(allocator);
    }

    /**
     * called by assignment to assign a new value in the scope that this variable was declared
     */
    void declare(Value *new_value);

};