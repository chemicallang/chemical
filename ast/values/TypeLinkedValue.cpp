// Copyright (c) Qinetik 2024.

#include "TypeLinkedValue.h"
#include "ast/statements/VarInit.h"
#include "ast/statements/Assignment.h"
#include "ast/statements/Return.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/StructValue.h"
#include "ast/types/FunctionType.h"
#include "ast/values/IntValue.h"

void TypeLinkedValue::link(SymbolResolver &linker, VarInitStatement *stmnt) {
    if(stmnt->type.has_value()) {
        link(linker, stmnt->value.value(), stmnt->type->get());
    } else {
        link(linker, stmnt->value.value(), (BaseType*) nullptr);
    }
}

void TypeLinkedValue::link(SymbolResolver &linker, AssignStatement *stmnt, bool lhs) {
    auto value_type = stmnt->lhs->create_type();
    link(linker, lhs ? stmnt->lhs : stmnt->value, value_type.get());
}

void TypeLinkedValue::link(SymbolResolver &linker, ReturnStatement *returnStmt) {
    if(returnStmt->func_type && returnStmt->func_type->returnType) {
        link(linker, returnStmt->value.value(), returnStmt->func_type->returnType.get());
    } else {
        link(linker, returnStmt->value.value(), (BaseType*) nullptr);
    }
}

void TypeLinkedValue::link(SymbolResolver &linker, FunctionCall *call, unsigned int index) {
    auto funcType = call->get_function_type();
    if(funcType) {
        link(linker, call->values[index], funcType->func_param_for_arg_at(index)->type.get());
    } else {
        link(linker, call->values[index], (BaseType*) nullptr);
    }
}

void TypeLinkedValue::link(SymbolResolver &linker, StructValue *structValue, const std::string &name) {
    auto child = structValue->definition->child(name);
    auto child_type = child->get_value_type();
    link(linker, structValue->values[name], child_type.get());
}