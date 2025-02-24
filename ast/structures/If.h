// Copyright (c) Chemical Language Foundation 2025.

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
            SourceLocation location
    ) : ASTNode(ASTNodeKind::IfStmt, location), Value(ValueKind::IfValue, location), condition(condition), ifBody(std::move(ifBody)),
        elseIfs(std::move(elseIfs)), elseBody(std::move(elseBody)), parent_node(parent_node),
        is_value(is_value) {}


    void set_parent(ASTNode* new_parent) final {
        parent_node = new_parent;
    }

    ASTNode *parent() final {
        return parent_node;
    }

    bool compile_time_computable() final;

    bool link_conditions(SymbolResolver &linker);

    bool is_top_level() {
        return !parent_node || parent_node->kind() == ASTNodeKind::NamespaceDecl;
    }

    void declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) final;

    void declare_and_link(SymbolResolver &linker, Value** value_ptr);

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final {
        declare_and_link(linker, nullptr);
    }

    bool link(SymbolResolver &linker, Value* &value_ptr, BaseType *expected_type = nullptr) final {
        declare_and_link(linker, &value_ptr);
        return true;
    }

    std::optional<bool> get_condition_const(InterpretScope& scope);

    Scope* get_evaluated_scope(InterpretScope& scope, ASTDiagnoser* gen, bool condition_value);

    Value* get_value_node();

    BaseType* create_type(ASTAllocator& allocator) final;

    BaseType* create_value_type(ASTAllocator& allocator) final;

    BaseType *known_type() final;

    ASTNode *linked_node() final;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen, bool gen_last_block);

    void code_gen(Codegen &gen) final;

    llvm::Type* llvm_type(Codegen &gen) final;

    llvm::AllocaInst* llvm_allocate(Codegen &gen, const std::string &identifier, BaseType *expected_type) final;

    void llvm_assign_value(Codegen &gen, llvm::Value *lhsPtr, Value *lhs) final;

    llvm::Value* llvm_value(Codegen &gen, BaseType *type = nullptr) final;

    void code_gen(Codegen &gen, Scope* scope, unsigned int index) final;

    bool add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) final;

#endif

    void interpret(InterpretScope &scope) final;

};