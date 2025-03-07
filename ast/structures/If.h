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
    bool is_value = false;
    bool is_computable = false;
    bool resolved_condition = true;
    // after resolving computed value, we store the scope, so we can visit it
    std::optional<Scope*> computed_scope = std::nullopt;

    /**
     * constructor
     */
    constexpr IfStatement(
            Value* condition,
            ASTNode* parent_node,
            bool is_value,
            SourceLocation location
    ) : ASTNode(ASTNodeKind::IfStmt, parent_node, location), Value(ValueKind::IfValue, location), condition(condition), ifBody(this, location),
        elseBody(std::nullopt),
        is_value(is_value) {}

    IfStatement* copy(ASTAllocator &allocator) override {
        const auto stmt = new (allocator.allocate<IfStatement>()) IfStatement(
            condition->copy(allocator),
            parent(),
            is_value,
            ASTNode::encoded_location()
        );
        stmt->is_value = is_value;
        stmt->is_computable = is_computable;
        stmt->resolved_condition = resolved_condition;
        ifBody.copy_into(stmt->ifBody, allocator, parent());
        stmt->elseIfs.reserve(elseIfs.size());
        for(auto& elif : elseIfs) {
            stmt->elseIfs.emplace_back(elif.first->copy(allocator), Scope(elif.second.parent(), elif.second.encoded_location()));
            auto& last = stmt->elseIfs.back();
            elif.second.copy_into(last.second, allocator, stmt);
        }
        if(elseBody.has_value()) {
            stmt->elseBody.emplace(elseBody->parent(), elseBody->encoded_location());
            elseBody->copy_into(stmt->elseBody.value(), allocator, stmt);
        }
        return stmt;
    }

    bool compile_time_computable() final;

    bool link_conditions(SymbolResolver &linker);

    bool is_top_level() {
        return !parent() || parent()->kind() == ASTNodeKind::NamespaceDecl;
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