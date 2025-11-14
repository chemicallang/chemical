// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "Scope.h"
#include "ast/base/Value.h"
#include <optional>

struct IfStatementAttrs {

    bool is_comptime = false;

};

class IfStatement : public ASTNode {
public:

    Value* condition;
    Scope ifBody;
    std::vector<std::pair<Value*, Scope>> elseIfs;
    std::optional<Scope> elseBody;
    IfStatementAttrs attrs;
    // after resolving computed value, we store the scope, so we can visit it
    std::optional<Scope*> computed_scope = std::nullopt;

    /**
     * constructor
     */
    constexpr IfStatement(
            Value* condition,
            ASTNode* parent_node,
            SourceLocation location
    ) : ASTNode(ASTNodeKind::IfStmt, parent_node, location), condition(condition), ifBody(this, location),
        elseBody(std::nullopt) {}

    /**
     * is comptime
     */
    inline bool is_comptime() const noexcept {
        return attrs.is_comptime;
    }

    void copy_into(ASTAllocator& allocator, IfStatement* stmt) {
        stmt->attrs = attrs;
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
    }

    IfStatement* copy(ASTAllocator &allocator) override {
        const auto stmt = new (allocator.allocate<IfStatement>()) IfStatement(
            condition->copy(allocator),
            parent(),
            ASTNode::encoded_location()
        );
        copy_into(allocator, stmt);
        return stmt;
    }

    bool compile_time_computable();

    void link_conditions(SymbolResolver &linker);

    /**
     * std::nullopt means the if statement couldn't be evaluated at comptile time
     */
    std::optional<Scope*> resolve_evaluated_scope(InterpretScope& comptime_scope, ASTDiagnoser& diagnoser);

    /**
     * get evaluated scope or resolve it if it can be resolved
     */
    std::optional<Scope*> get_or_resolve_scope(InterpretScope& comptime_scope, ASTDiagnoser& diagnoser) {
        if(computed_scope.has_value()) {
            return computed_scope;
        } else if(is_comptime()) {
            return resolve_evaluated_scope(comptime_scope, diagnoser);
        } else {
            return std::nullopt;
        }
    }

    Scope* link_evaluated_scope(SymbolResolver& linker);

    /**
     * first the conditions are linked, then the scope is determined by
     * evaluating conditions
     */
    inline Scope* get_evaluated_scope_by_linking(SymbolResolver& linker) {
        return link_evaluated_scope(linker);
    }

    std::optional<bool> get_condition_const(InterpretScope& scope);

    Scope* get_evaluated_scope(InterpretScope& scope, ASTDiagnoser* gen, bool condition_value);

    Value* get_value_node();

    BaseType *known_type();

#ifdef COMPILER_BUILD

    void code_gen_declare(Codegen &gen) override;

    void code_gen(Codegen &gen, bool gen_last_block);

    void code_gen(Codegen &gen) final;

    void code_gen(Codegen &gen, Scope* scope, unsigned int index) final;

    void code_gen_external_declare(Codegen &gen) override;

#endif

};