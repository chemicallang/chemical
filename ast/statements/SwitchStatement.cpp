// Copyright (c) Chemical Language Foundation 2025.

#include "SwitchStatement.h"
#include "ast/base/Value.h"
#include "ast/values/AccessChain.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/VariantCaseVariable.h"
#include "ast/structures/VariantMember.h"
#include "ast/structures/GenericVariantDecl.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/values/VariantCase.h"
#include "ast/values/SwitchValue.h"
#include "ast/types/ReferenceType.h"
#include "ast/types/IntNType.h"
#include "ast/structures/VariantDefinition.h"


Value* SwitchStatement::get_value_node() {
    return Value::get_first_value_from_value_node(this);
}

BaseType *SwitchStatement::known_type() {
    if(scopes.empty()) return nullptr;
    auto last_val = get_value_node();
    return last_val ? last_val->getType() : nullptr;
}

ASTNode *SwitchValue::linked_node() {
    const auto known = stmt.known_type();
    return known ? known->linked_node() : nullptr;
}

VariantDefinition* SwitchStatement::getVarDefFromExpr() {
    const auto expr_type = expression->getType();
    if(expr_type) {
        const auto linked = expr_type->linked_node();
        if(linked) {
            const auto kind = linked->kind();
            if(kind == ASTNodeKind::VariantMember) {
                const auto member = linked->as_variant_member_unsafe();
                return member->parent();
            } else if(kind == ASTNodeKind::VariantDecl) {
                return linked->as_variant_def_unsafe();
            } else if(kind == ASTNodeKind::GenericVariantDecl) {
                return linked->as_gen_variant_decl_unsafe()->master_impl;
            }
        }
    }
    return nullptr;
}

void SwitchStatement::copy_into(ASTAllocator& allocator, SwitchStatement* stmt) {
    stmt->cases.reserve(cases.size());
    for(auto& aCase : cases) {
        const auto copied_case = aCase.first->copy(allocator);
        if(copied_case->kind() == ValueKind::VariantCase) {
            // since this is a variant case, we must ensure its linked properly
            const auto var_case = copied_case->as_variant_case_unsafe();
            // use the copied switch
            var_case->switch_statement = stmt;
            // copy the variables
            unsigned i = 0;
            const auto total_vars = var_case->identifier_list.size();
            while(i < total_vars) {
                auto& var = var_case->identifier_list[i];
                var = var->copy(allocator);
                var->set_parent(stmt);
                i++;
            }
        }
        stmt->cases.emplace_back(copied_case, aCase.second);
    }
    stmt->scopes.reserve(scopes.size());
    for(auto& scope : scopes) {
        stmt->scopes.emplace_back(scope.parent(), scope.encoded_location());
        scope.copy_into(stmt->scopes.back(), allocator, stmt);
    }
}

SwitchStatement* SwitchStatement::copy(ASTAllocator &allocator) {
    const auto stmt = new (allocator.allocate<SwitchStatement>()) SwitchStatement(
            expression->copy(allocator),
            parent(),
            ASTNode::encoded_location()
    );
    copy_into(allocator, stmt);
    stmt->defScopeInd = defScopeInd;
    return stmt;
}