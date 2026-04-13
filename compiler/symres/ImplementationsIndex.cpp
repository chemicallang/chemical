// Copyright (c) Chemical Language Foundation 2026.

#include "ImplementationsIndex.h"
#include "CoreNodes.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/ImplDefinition.h"

static FunctionDeclaration* implementation_of(ImplementationsIndex& index, FunctionDeclaration* op_base, ASTAny* for_type) {
    if (op_base == nullptr) {
        return nullptr;
    }
    const auto implBlock = index.get(op_base->parent(), for_type);
    if (implBlock == nullptr) {
        return nullptr;
    }
    return implBlock->implementation_of(op_base);
}

FunctionDeclaration* ImplementationsIndex::get_expr_op_impl(CoreNodes& coreNodes, MembersContainer* container, Operation op) {
    return implementation_of(*this, coreNodes.expr_operator_impl_base(op), (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_ass_op_impl(CoreNodes& coreNodes, MembersContainer* container, Operation op) {
    return implementation_of(*this, coreNodes.assignment_operator_impl_base(op), (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_inc_dec_op_impl(CoreNodes& coreNodes, MembersContainer* container, bool increment, bool post) {
    return implementation_of(*this, coreNodes.inc_dec_operator_impl_base(increment, post), (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_index_op_impl(CoreNodes& coreNodes, MembersContainer* container) {
    const auto indexFunc = coreNodes.ops.index;
    if (indexFunc != nullptr) {
        const auto implPresent = get(indexFunc->parent(), container);
        if (implPresent) {
            return implPresent->implementation_of(indexFunc);
        }
    }
    const auto mutIndexFunc = coreNodes.ops.index_mut;
    if (mutIndexFunc) {
        const auto implPresent = get(mutIndexFunc->parent(), container);
        if (implPresent) {
            return implPresent->implementation_of(mutIndexFunc);
        }
    }
    return nullptr;
}

FunctionDeclaration* ImplementationsIndex::get_neg_op_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.ops.neg, (ASTAny*) container);
}

FunctionDeclaration* ImplementationsIndex::get_not_op_impl(CoreNodes& coreNodes, MembersContainer* container) {
    return implementation_of(*this, coreNodes.ops._not, (ASTAny*) container);
}