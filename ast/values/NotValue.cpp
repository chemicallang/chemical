// Copyright (c) Chemical Language Foundation 2025.

#include "ast/base/BaseType.h"
#include "ast/base/InterpretScope.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/base/TypeBuilder.h"
#include "ast/base/ASTNode.h"
#include "ast/structures/MembersContainer.h"
#include "NotValue.h"
#include "BoolValue.h"
#include "NullValue.h"

bool NotValue::primitive() {
    return false;
}

Value* NotValue::evaluated_value(InterpretScope &scope) {
    const auto val = value->evaluated_value(scope);
    if(val->val_kind() == ValueKind::Bool) {
        return new (scope.allocate<BoolValue>()) BoolValue(!val->as_bool_unsafe()->value, scope.global->typeBuilder.getBoolType(), encoded_location());
    } else {
        scope.error("couldn't evaluate as value didn't return a boolean value", this);
        return new (scope.allocate<NullValue>()) NullValue(scope.global->typeBuilder.getNullPtrType(), encoded_location());
    };
}

void verify_bool_ptr_condition(ASTDiagnoser& linker, BaseType* condType, SourceLocation loc);

void NotValue::determine_type(ASTDiagnoser& diagnoser) {
    const auto type = getValue()->getType();
    // check if operator is overloaded
    const auto node = type->get_linked_canonical_node(true, false);
    if(node) {
        const auto container = node->get_members_container();
        if(container) {
            const auto child = container->child("not");
            if(!child || child->kind() != ASTNodeKind::FunctionDecl) {
                diagnoser.error(this) << "expected a function 'not' to be present for overloading";
                setType(type);
                return;
            }
            const auto func = child->as_function_unsafe();
            if(func->params.size() != 1) {
                diagnoser.error(this) << "expected 'not' operator function to have a single parameter";
                setType(type);
                return;
            }
            setType(func->returnType);
            return;
        }
    }
    // normal flow
    setType(type);
    // check not value type is proper
    verify_bool_ptr_condition(diagnoser, type, encoded_location());
}