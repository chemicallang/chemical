// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/Value.h"

class TypeLinkedValue  {
public:

    virtual bool link(SymbolResolver& linker, std::unique_ptr<Value>& value_ptr, BaseType* type) = 0;

    bool link(SymbolResolver &linker, StructValue *value, const std::string &name);

    bool link(SymbolResolver& linker, ArrayValue* value, unsigned int index);

    bool link(SymbolResolver &linker, FunctionCall *call, unsigned int index);

    bool link(SymbolResolver &linker, ReturnStatement *returnStmt);

};