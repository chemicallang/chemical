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
#include "compiler/SymbolResolver.h"

bool TypeLinkedValue::link(SymbolResolver &linker, VarInitStatement *stmnt) {
    return link(linker, stmnt->value, stmnt->type ? stmnt->type.get() : nullptr);
}

bool TypeLinkedValue::link(SymbolResolver &linker, AssignStatement *stmnt, bool lhs) {
    std::unique_ptr<BaseType> value_type;
    if(!lhs) {
        value_type = stmnt->lhs->create_type();
    }
    return link(linker, lhs ? stmnt->lhs : stmnt->value, lhs ? nullptr : value_type.get());
}

BaseType* implicit_constructor_type(BaseType* return_type, Value* value) {
    auto k = return_type->kind();
    if(k == BaseTypeKind::Linked || k == BaseTypeKind::Generic) {
        const auto linked = return_type->linked_node();
        const auto struc = linked->as_struct_def();
        if(struc) {
            const auto constr = struc->implicit_constructor_for(value);
            if(constr) {
                return constr->func_param_for_arg_at(0)->type.get();
            }
        }
    }
    return return_type;
}

bool TypeLinkedValue::link(SymbolResolver &linker, ReturnStatement *returnStmt) {
    return link(linker, returnStmt->value, returnStmt->func_type && returnStmt->func_type->returnType ? returnStmt->func_type->returnType.get() : nullptr);
}

bool TypeLinkedValue::link(SymbolResolver &linker, FunctionCall *call, unsigned int index) {
    return link(linker, call->values[index], call->get_arg_type(index));
}

bool TypeLinkedValue::link(SymbolResolver &linker, StructValue *structValue, const std::string &name) {
    auto child = structValue->child(name);
    if(!child) {
        linker.error("couldn't find child " + name + " in struct declaration", structValue);
        return false;
    }
    auto child_type = child->get_value_type();
    return link(linker, structValue->values[name]->value, child_type.get());
}

bool TypeLinkedValue::link(SymbolResolver& linker, ArrayValue* value, unsigned int index) {
    return link(linker, value->values[index], value->elemType ? value->elemType.get() : nullptr);
}