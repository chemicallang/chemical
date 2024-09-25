// Copyright (c) Qinetik 2024.

#pragma once

#include "CBIUtils.h"

class CSTConverter;

class Value;

class BaseType;

class ASTNode;

class CSTToken;

extern "C" {

    void CSTConverterput_value(CSTConverter* converter, Value* value, CSTToken* token);

    void CSTConverterput_node(CSTConverter* converter, ASTNode* node, CSTToken* token);

    void CSTConverterput_type(CSTConverter* converter, BaseType* type, CSTToken* token);

    Value* CSTConverterpop_last_value(CSTConverter* converter);

    ASTNode* CSTConverterpop_last_node(CSTConverter* converter);

    BaseType* CSTConverterpop_last_type(CSTConverter* converter);

    void CSTConvertervisit(CSTConverter* converter, CSTToken* token);

}