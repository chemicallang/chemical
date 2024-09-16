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

    std::unique_ptr<Value> condition;
    Scope ifBody;
    std::vector<std::pair<std::unique_ptr<Value>, Scope>> elseIfs;
    std::optional<Scope> elseBody;
    ASTNode* parent_node;
    bool is_value;
    CSTToken* token;
    bool is_computable = false;

    /**
     * @brief Construct a new IfStatement object.
     *
     * @param condition The condition of the if statement.
     * @param ifBody The body of the if statement.
     * @param elseBody The body of the else statement (can be nullptr if there's no else part).
     */
    IfStatement(
            std::unique_ptr<Value> condition,
            Scope ifBody,
            std::vector<std::pair<std::unique_ptr<Value>, Scope>> elseIfs,
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

    void link_conditions(SymbolResolver &linker);

    bool is_top_level() {
        return !parent_node || parent_node->kind() == ASTNodeKind::NamespaceDecl;
    }

    void declare_top_level(SymbolResolver &linker, std::unique_ptr<ASTNode> &node_ptr) override;

    void declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>* node_ptr, std::unique_ptr<Value>* value_ptr);

    void declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) override {
        declare_and_link(linker, &node_ptr, nullptr);
    }

    bool link(SymbolResolver &linker, std::unique_ptr<Value> &value_ptr, BaseType *expected_type = nullptr) override {
        declare_and_link(linker, nullptr, &value_ptr);
        return true;
    }

    Scope* get_evaluated_scope(InterpretScope& scope, ASTDiagnoser* gen);

    Value* get_value_node();

    std::unique_ptr<BaseType> create_type() override;

    std::unique_ptr<BaseType> create_value_type() override;

    BaseType *known_type() override;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen, bool gen_last_block);

    void code_gen(Codegen &gen) override;

    llvm::Type* llvm_type(Codegen &gen) override;

    llvm::AllocaInst* llvm_allocate(Codegen &gen, const std::string &identifier, BaseType *expected_type) override;

    llvm::Value* llvm_assign_value(Codegen &gen, Value *lhs) override;

    llvm::Value* llvm_value(Codegen &gen, BaseType *type = nullptr) override;

    void code_gen(Codegen &gen, Scope* scope, unsigned int index) override;

#endif

    void interpret(InterpretScope &scope) override;

};