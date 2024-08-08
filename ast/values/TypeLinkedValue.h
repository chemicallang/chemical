// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/Value.h"

class TypeLinkedValue : public Value {
public:

    virtual void link(SymbolResolver& linker, std::unique_ptr<Value>& value_ptr, BaseType* type) = 0;

    void link(SymbolResolver &linker, VarInitStatement *stmnt) override;

    void link(SymbolResolver &linker, AssignStatement *stmnt, bool lhs) override;

    void link(SymbolResolver &linker, StructValue *value, const std::string &name) override;

    void link(SymbolResolver &linker, FunctionCall *call, unsigned int index) override;

    void link(SymbolResolver &linker, ReturnStatement *returnStmt) override;

};