// Copyright (c) Qinetik 2024.

#include "TypeLinkedValue.h"
#include "ast/statements/VarInit.h"
#include "ast/statements/Assignment.h"
#include "ast/statements/Return.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/StructValue.h"
#include "ast/values/ArrayValue.h"
#include "ast/types/FunctionType.h"
#include "ast/values/IntValue.h"

void TypeLinkedValue::link(SymbolResolver &linker, VarInitStatement *stmnt) {
    link(linker, stmnt->value, stmnt->type ? stmnt->type.get() : nullptr);
}

void TypeLinkedValue::link(SymbolResolver &linker, AssignStatement *stmnt, bool lhs) {
    std::unique_ptr<BaseType> value_type;
    if(!lhs) {
        value_type = stmnt->lhs->create_type();
    }
    link(linker, lhs ? stmnt->lhs : stmnt->value, lhs ? nullptr : value_type.get());
}

void TypeLinkedValue::link(SymbolResolver &linker, ReturnStatement *returnStmt) {
    link(linker, returnStmt->value, returnStmt->func_type && returnStmt->func_type->returnType ? returnStmt->func_type->returnType.get() : nullptr);
}

void TypeLinkedValue::link(SymbolResolver &linker, FunctionCall *call, unsigned int index) {
    link(linker, call->values[index], call->get_arg_type(index));
}

void TypeLinkedValue::link(SymbolResolver &linker, StructValue *structValue, const std::string &name) {
    auto child = structValue->definition->child(name);
    auto child_type = child->get_value_type();
    link(linker, structValue->values[name], child_type.get());
}

void TypeLinkedValue::link(SymbolResolver& linker, ArrayValue* value, unsigned int index) {
    link(linker, value->values[index], value->elemType ? value->elemType.get() : nullptr);
}