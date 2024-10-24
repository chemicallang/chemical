// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "Scope.h"
#include "ast/base/Value.h"
#include <optional>

class IfStatement : public ASTNode, public Value {
public:

    Value* condition;
    Scope ifBody;
    std::vector<std::pair<Value*, Scope>> elseIfs;
    std::optional<Scope> elseBody;
    ASTNode* parent_node;
    bool is_value;
    CSTToken* token;
    bool is_computable = false;
    bool resolved_condition = true;
    // after resolving computed value, we store the scope, so we can visit it
    std::optional<Scope*> computed_scope = std::nullopt;

    /**
     * @brief Construct a new IfStatement object.
     *
     * @param condition The condition of the if statement.
     * @param ifBody The body of the if statement.
     * @param elseBody The body of the else statement (can be nullptr if there's no else part).
     */
    IfStatement(
            Value* condition,
            Scope ifBody,
            std::vector<std::pair<Value*, Scope>> elseIfs,
            std::optional<Scope> elseBody,
            ASTNode* parent_node,
            bool is_value,
            CSTToken* token
    );

    CSTToken *cst_token() override {
        return token;
    }

    ValueKind val_kind() override {
        return ValueKind::IfValue;
    }

    ASTNodeKind kind() override {
        return ASTNodeKind::IfStmt;
    }

    void set_parent(ASTNode* new_parent) override {
        parent_node = new_parent;
    }

    ASTNode *parent() override {
        return parent_node;
    }

    void accept(Visitor *visitor) override;

    bool compile_time_computable() override;

    bool link_conditions(SymbolResolver &linker);

    bool is_top_level() {
        return !parent_node || parent_node->kind() == ASTNodeKind::NamespaceDecl;
    }

    void declare_top_level(SymbolResolver &linker) override;

    void declare_and_link(SymbolResolver &linker, Value** value_ptr);

    void declare_and_link(SymbolResolver &linker) override {
        declare_and_link(linker, nullptr);
    }

    bool link(SymbolResolver &linker, Value* &value_ptr, BaseType *expected_type = nullptr) override {
        declare_and_link(linker, &value_ptr);
        return true;
    }

    std::optional<bool> get_condition_const(InterpretScope& scope);

    Scope* get_evaluated_scope(InterpretScope& scope, ASTDiagnoser* gen, bool condition_value);

    Value* get_value_node();

    BaseType* create_type(ASTAllocator& allocator) override;

    BaseType* create_value_type(ASTAllocator& allocator) override;

    BaseType *known_type() override;

    ASTNode *linked_node() override;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen, bool gen_last_block);

    void code_gen(Codegen &gen) override;

    llvm::Type* llvm_type(Codegen &gen) override;

    llvm::AllocaInst* llvm_allocate(Codegen &gen, const std::string &identifier, BaseType *expected_type) override;

    llvm::Value* llvm_assign_value(Codegen &gen, llvm::Value *lhsPtr, Value *lhs) override;

    llvm::Value* llvm_value(Codegen &gen, BaseType *type = nullptr) override;

    void code_gen(Codegen &gen, Scope* scope, unsigned int index) override;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) override;

#endif

    void interpret(InterpretScope &scope) override;

};