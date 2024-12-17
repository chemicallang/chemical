// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"
#include "ast/base/AnnotableNode.h"
#include "parser/model/tokens/NumberToken.h"
#include <optional>
#include "ast/base/BaseType.h"
#include "ast/base/AccessSpecifier.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/base/LocatedIdentifier.h"

struct VarInitExtData {

    /**
     * the access specifier of the declaration
     */
    AccessSpecifier specifier;

    /**
     * is var init comptime
     */
    bool is_comptime;

    /**
     * has moved is used to indicate that an object at this location has moved
     * destructor is not called on moved objects, once moved, any attempt to access
     * this variable causes an error
     */
    bool has_moved = false;
    /**
     * has moved is used to track that var init statement has a assignment to it
     * if it has, during symbol resolution the assignment statement notifies this
     * var init statement
     */
    bool has_assignment = false;

    /**
     * this tells whether wrote const or var when creating the variable
     */
    bool is_const;

};

class VarInitStatement : public AnnotableNode {
public:

    // TODO do not store the decl_scope here
    InterpretScope *decl_scope = nullptr;
    LocatedIdentifier located_id; ///< The identifier being initialized.
    BaseType* type;
    Value* value; ///< The value being assigned to the identifier.
    ASTNode* parent_node;
    SourceLocation location;
#ifdef COMPILER_BUILD
    llvm::Value *llvm_ptr;
#endif

    /**
     * extra data for the var init
     */
    VarInitExtData data;

    /**
     * constructor
     */
    VarInitStatement(
            bool is_const,
            LocatedIdentifier id,
            BaseType* type,
            Value* value,
            ASTNode* parent_node,
            SourceLocation location,
            AccessSpecifier specifier = AccessSpecifier::Internal
    );

    /**
     * get the name / identifier of the declaration
     */
    inline const std::string name() {
        return located_id.identifier.str();
    }

    /**
     * get the name / identifier of the declaration
     */
    inline const std::string identifier() {
        return located_id.identifier.str();
    }

    /**
     * get the name / identifier of the declaration
     */
    inline const chem::string_view& id_view() {
        return located_id.identifier;
    }

    inline bool is_comptime() {
        return data.is_comptime;
    }

    inline void set_comptime(bool value) {
        data.is_comptime = value;
    }

    /**
     * get the access specifier
     */
    inline AccessSpecifier specifier() const {
        return data.specifier;
    }

    /**
     * set's the specifier of this decl fast
     */
    inline void set_specifier(AccessSpecifier specifier) {
        data.specifier = specifier;
    }

    /**
     * check is this declaration const
     */
    inline bool is_const() {
        return data.is_const;
    }

    /**
     * check this variable has been moved
     */
    inline bool get_has_moved() const {
        return data.has_moved;
    }

    /**
     * call it when this variable has been moved
     */
    inline void moved() {
        data.has_moved = true;
    }

    /**
     * call it when this variable should be unmoved
     */
    inline void unmove() {
        data.has_moved = false;
    }

    /**
     * get has assignment
     */
    inline bool get_has_assignment() {
        return data.has_assignment;
    }

    /**
     * assignment can be set to true
     */
    inline void set_has_assignment() {
        data.has_assignment = true;
    }

    SourceLocation encoded_location() final {
        return location;
    }

    ASTNodeKind kind() final {
        return ASTNodeKind::VarInitStmt;
    }

    void set_parent(ASTNode* new_parent) final {
        parent_node = new_parent;
    }

    ASTNode *parent() final {
        return parent_node;
    }

    void accept(Visitor *visitor) final;

    Value* holding_value() final {
        return value;
    }

    BaseType* known_type() final;

    bool is_top_level();

    BaseType* type_ptr_fast() {
        return type;
    }

    const std::string ns_node_identifier() final {
        return identifier();
    }

#ifdef COMPILER_BUILD

    inline void check_has_type(Codegen &gen);

    llvm::Value *llvm_pointer(Codegen &gen) final;

    llvm::Type *llvm_elem_type(Codegen &gen) final;

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) final;

    llvm::Value *llvm_load(Codegen &gen) final;

    llvm::FunctionType *llvm_func_type(Codegen &gen) final;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) final;

    void code_gen_global_var(Codegen &gen, bool initialize);

    void code_gen(Codegen &gen) final;

    void code_gen_destruct(Codegen &gen, Value* returnValue) final;

    void code_gen_external_declare(Codegen &gen) final;

#endif

    void runtime_name_no_parent(std::ostream &stream) final {
        stream << identifier();
    }

    inline std::string runtime_name_fast() {
        return parent_node ? runtime_name_str() : identifier();
    }

    ASTNode *child(const std::string &name) final;

    void declare_top_level(SymbolResolver &linker) final;

    void declare_and_link(SymbolResolver &linker) final;

    void interpret(InterpretScope &scope) final;

    BaseType* create_value_type(ASTAllocator& allocator) final;

//    hybrid_ptr<BaseType> get_value_type() final;

    /**
     * called by assignment to assign a new value in the scope that this variable was declared
     */
    void declare(Value *new_value);

    [[nodiscard]]
    ValueType value_type() const final;

    [[nodiscard]]
    BaseTypeKind type_kind() const final;

};