// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/Value.h"

class TypeLinkedValue  {
public:

    virtual bool link(SymbolResolver& linker, std::unique_ptr<Value>& value_ptr, BaseType* type) = 0;

};