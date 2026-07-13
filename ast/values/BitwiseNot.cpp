// Copyright (c) Chemical Language Foundation 2025.

#include "ast/base/BaseType.h"
#include "ast/base/InterpretScope.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/base/TypeBuilder.h"
#include "ast/base/ASTNode.h"
#include "ast/structures/MembersContainer.h"
#include "BitwiseNot.h"
#include "IntNumValue.h"
#include "ast/types/IntNType.h"
#include "compiler/ASTDiagnoser.h"
#include "compiler/symres/ImplementationsIndex.h"

bool BitwiseNot::primitive() {
    return value->primitive();
}

Value* BitwiseNot::evaluated_value(InterpretScope &scope) {
    const auto eval = value->evaluated_value(scope);
    if(eval->val_kind() == ValueKind::IntN) {
        const auto num = eval->as_int_num_value_unsafe();
        return new (scope.allocate<IntNumValue>()) IntNumValue(~num->get_num_value(), num->getType(), encoded_location());
    } else {
        scope.error("couldn't evaluate as value didn't return an integer value", this);
        return nullptr;
    }
}

void BitwiseNot::determine_type(ASTDiagnoser& diagnoser, const CoreNodes& coreNodes, const ImplementationsIndex& implsIndex) {
    const auto type = getValue()->getType();
    const auto node = type->get_linked_canonical_node(true, false);
    if(node) {
        const auto container = node->get_members_container();
        if(container) {
            const auto func = implsIndex.get_bitnot_op_impl(coreNodes, container);
            if (func == nullptr) {
                diagnoser.error(this) << "expected 'bitnot' operator overloaded function implementation";
                setType(type);
                return;
            }
            if(func->params.size() != 1) {
                diagnoser.error(this) << "expected 'bitnot' operator function to have a single parameter";
                setType(type);
                return;
            }
            setType(func->returnType);
            return;
        }
    }
    setType(type);
}
