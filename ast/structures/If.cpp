// Copyright (c) Chemical Language Foundation 2025.

#include "If.h"
#include "ast/base/BaseType.h"
#include "ast/types/FunctionType.h"
#include "ast/base/InterpretScope.h"
#include "compiler/ASTDiagnoser.h"
#include "ast/values/IfValue.h"


std::optional<bool> IfStatement::get_condition_const(InterpretScope& scope) {
    auto constant = condition->evaluated_value(scope);
    if(!constant || constant->val_kind() != ValueKind::Bool) {
        return std::nullopt;
    }
    return constant->get_the_bool();
}

Scope* IfStatement::get_evaluated_scope(InterpretScope& scope, ASTDiagnoser* gen, bool condition_value) {
    if(condition_value) {
        return &ifBody;
    } else {
        for(auto& elseIf : elseIfs) {
            auto constant = elseIf.first->evaluated_value(scope);
            if(!constant || constant->val_kind() != ValueKind::Bool) {
                gen->error("couldn't get constant value for top level if statement's condition", (ASTNode*) this);
                return nullptr;
            }
            if(constant->get_the_bool()) {
                return &elseIf.second;
            }
        }
        if(elseBody.has_value()) {
            return &elseBody.value();
        }
    };
    return nullptr;
}

bool IfStatement::compile_time_computable() {
    if(!condition->compile_time_computable()) {
        return false;
    }
    for(auto& elseIf : elseIfs) {
        if(!elseIf.first->compile_time_computable()) {
            return false;
        }
    }
    return true;
}

std::optional<Scope*> IfStatement::resolve_evaluated_scope(InterpretScope& comptime_scope, ASTDiagnoser& diagnoser) {
    auto condition_val = get_condition_const(comptime_scope);
    if(condition_val.has_value()) {
        return get_evaluated_scope(comptime_scope, &diagnoser, condition_val.value());
    } else {
        return std::nullopt;
    }
}

Value* IfStatement::get_value_node() {
    return Value::get_first_value_from_value_node(this);
}

BaseType *IfStatement::known_type() {
    auto last_val = get_value_node();
    return last_val ? last_val->getType() : nullptr;
}

ASTNode *IfValue::linked_node() {
    const auto known = stmt.known_type();
    return known ? known->linked_node() : nullptr;
}