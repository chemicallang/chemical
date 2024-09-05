// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"
#include "ast/base/AnnotableNode.h"
#include "lexer/model/tokens/NumberToken.h"
#include <optional>
#include "ast/base/BaseType.h"
#include "ast/base/AccessSpecifier.h"
#include "ast/base/GlobalInterpretScope.h"

class VarInitStatement : public AnnotableNode {
public:

    bool is_const;
    InterpretScope *decl_scope = nullptr;
    std::string identifier; ///< The identifier being initialized.
    std::unique_ptr<BaseType> type;
    std::unique_ptr<Value> value; ///< The value being assigned to the identifier.
    ASTNode* parent_node;
    CSTToken* token;
    AccessSpecifier specifier;
    /**
     * has moved is used to indicate that an object at this location has moved
     * destructor is not called on moved objects, once moved, any attempt to access
     * this variable causes an error
     */
    bool has_moved = false;

    /**
     * constructor
     */
    VarInitStatement(
            bool is_const,
            std::string identifier,
            std::unique_ptr<BaseType> type,
            std::unique_ptr<Value> value,
            ASTNode* parent_node,
            CSTToken* token,
            AccessSpecifier specifier = AccessSpecifier::Internal
    );

    CSTToken *cst_token() override {
        return token;
    }

    ASTNodeKind kind() override {
        return ASTNodeKind::VarInitStmt;
    }

    void set_parent(ASTNode* new_parent) override {
        parent_node = new_parent;
    }

    ASTNode *parent() override {
        return parent_node;
    }

    void accept(Visitor *visitor) override;

    Value* holding_value() override {
        return value ? value.get() : nullptr;
    }

    BaseType* known_type() override;

    bool is_top_level();

#ifdef COMPILER_BUILD

    const std::string& ns_node_identifier() override {
        return identifier;
    }

    inline void check_has_type(Codegen &gen);

    llvm::Value *llvm_pointer(Codegen &gen) override;

    llvm::Type *llvm_elem_type(Codegen &gen) override;

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<std::unique_ptr<ChainValue>> &values, unsigned int index) override;

    llvm::Value *llvm_load(Codegen &gen) override;

    llvm::FunctionType *llvm_func_type(Codegen &gen) override;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) override;

    void code_gen(Codegen &gen) override;

    void code_gen_destruct(Codegen &gen, Value* returnValue) override;

#endif

    ASTNode *child(const std::string &name) override;

    void declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) override;

    void interpret(InterpretScope &scope) override;

    std::unique_ptr<BaseType> create_value_type() override;

    hybrid_ptr<BaseType> get_value_type() override;

    /**
     * called by assignment to assign a new value in the scope that this variable was declared
     */
    void declare(Value *new_value);

    ValueType value_type() const override;

    BaseTypeKind type_kind() const override;

#ifdef COMPILER_BUILD
    llvm::Value *llvm_ptr;
#endif

};