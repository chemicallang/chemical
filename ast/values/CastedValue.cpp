// Copyright (c) Qinetik 2024.

#include "CastedValue.h"


CastedValue::CastedValue(
        std::unique_ptr<Value> value,
        std::unique_ptr<BaseType> type,
        CSTToken* token
) : value(std::move(value)), type(std::move(type)), token(token) {

}

CastedValue *CastedValue::copy() {
    return new CastedValue(
        std::unique_ptr<Value>(value->copy()),
        std::unique_ptr<BaseType>(type->copy()),
        token
    );
}

bool CastedValue::link(SymbolResolver &linker, std::unique_ptr<Value>& value_ptr, BaseType* expected_type) {
    value->link(linker, value);
    type->link(linker, type);
    return true;
}

ASTNode *CastedValue::linked_node() {
    return type->linked_node();
}