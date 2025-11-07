// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"
#include "ast/structures/Scope.h"
#include <optional>

struct SwitchStatementAttrs {

    /**
     * set during symbol resolution, when switching on a variant or enum which is closed
     * when this is true, the default case is NOT required
     */
    bool operating_on_closed_value = false;

};

class SwitchStatement : public ASTNode {
public:

    Value* expression;
    std::vector<Scope> scopes;
    // cases store the index of scope, if value is nullptr it means default case
    std::vector<std::pair<Value*, int>> cases;
    int defScopeInd = -1;
    SwitchStatementAttrs attrs;

    constexpr SwitchStatement(
        Value* expression,
        ASTNode* parent_node,
        SourceLocation location
    ) : ASTNode(ASTNodeKind::SwitchStmt, parent_node, location), expression(expression) {

    }

    VariantDefinition* getVarDefFromExpr();

    void copy_into(ASTAllocator& allocator, SwitchStatement* stmt);

    SwitchStatement* copy(ASTAllocator &allocator) override;

    Value* get_value_node();

    BaseType *known_type() final;

    inline bool has_default_case() {
        return defScopeInd != -1;
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen, bool last_block);

    void code_gen(Codegen &gen) {
        code_gen(gen, false);
    }

    void code_gen(Codegen &gen, Scope* scope, unsigned int index) final;

#endif

};