// Copyright (c) Qinetik 2024.

#include "NumberValue.h"
#include "ast/statements/VarInit.h"
#include "ast/statements/Assignment.h"
#include "ast/statements/Return.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/StructValue.h"
#include "ast/types/FunctionType.h"
#include "ast/values/IntValue.h"

unsigned int NumberValue::get_num_bits() {
    if(linked_type == nullptr) {
        return 32;
    } else {
        return linked_type->num_bits();
    }
}

bool NumberValue::is_unsigned() {
    if(linked_type == nullptr) {
        return false;
    } else {
        return linked_type->is_unsigned();
    }
}

ValueType NumberValue::value_type() const {
    if(linked_type == nullptr) {
        return ValueType::Int;
    } else {
        return linked_type->value_type();
    }
}

Value *NumberValue::scope_value(InterpretScope &scope) {
    return new IntValue((int) value);
}

std::unique_ptr<IntNType> linked(BaseType* type) {
    auto pure = type->get_pure_type();
    if(pure->kind() == BaseTypeKind::IntN) {
        return std::unique_ptr<IntNType>(((IntNType*) pure->copy()));
    } else {
        return nullptr;
    }
}

void NumberValue::link(SymbolResolver &linker, VarInitStatement *stmnt) {
    if(stmnt->type.has_value()) {
        linked_type = linked(stmnt->type->get());
    }
}

void NumberValue::link(SymbolResolver &linker, AssignStatement *stmnt, bool lhs) {
    auto value_type = stmnt->lhs->create_type();
    linked_type = linked(value_type.get());
}

void NumberValue::link(SymbolResolver &linker, ReturnStatement *returnStmt) {
    if(returnStmt->func_type && returnStmt->func_type->returnType) {
        linked_type = linked(returnStmt->func_type->returnType.get());
    }
}

void NumberValue::link(SymbolResolver &linker, FunctionCall *call, unsigned int index) {
    auto value_type = call->parent_val->create_type();
    auto funcType = (FunctionType*) value_type.get();
    linked_type = linked(funcType->params[index]->type.get());
}

void NumberValue::link(SymbolResolver &linker, StructValue *structValue, const std::string &name) {
    auto child = structValue->definition->child(name);
    linked_type = linked(child->create_value_type().get());
}