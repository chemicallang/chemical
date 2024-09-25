// Copyright (c) Qinetik 2024.

#include "CSTConverterCBI.h"
#include "cst/base/CSTConverter.h"

void CSTConverterput_value(CSTConverter* converter, Value* value, CSTToken* token) {
    converter->put_value(value, token);
}

void CSTConverterput_node(CSTConverter* converter, ASTNode* node, CSTToken* token) {
    converter->put_node(node, token);
}

void CSTConverterput_type(CSTConverter* converter, BaseType* type, CSTToken* token) {
    converter->put_type(type, token);
}

Value* CSTConverterpop_last_value(CSTConverter* converter) {
    return converter->pop_last_value();
}

ASTNode* CSTConverterpop_last_node(CSTConverter* converter) {
    return converter->pop_last_node();
}

BaseType* CSTConverterpop_last_type(CSTConverter* converter) {
    return converter->pop_last_type();
}

void CSTConvertervisit(CSTConverter* converter, CSTToken* token) {
    token->accept(converter);
}