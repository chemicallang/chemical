// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/Value.h"

class TypeLinkedValue  {
public:

    virtual void link(SymbolResolver& linker, std::unique_ptr<Value>& value_ptr, BaseType* type) = 0;

    void link(SymbolResolver &linker, VarInitStatement *stmnt);

    void link(SymbolResolver &linker, AssignStatement *stmnt, bool lhs);

    void link(SymbolResolver &linker, StructValue *value, const std::string &name);

    void link(SymbolResolver& linker, ArrayValue* value, unsigned int index);

    void link(SymbolResolver &linker, FunctionCall *call, unsigned int index);

    void link(SymbolResolver &linker, ReturnStatement *returnStmt);

};