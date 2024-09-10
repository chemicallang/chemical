// Copyright (c) Qinetik 2024.

#pragma once

#include "LinkedType.h"
#include "ast/base/Value.h"

class LinkedValueType : public LinkedType {
public:

    std::unique_ptr<Value> value;

    LinkedValueType(std::unique_ptr<Value> value, CSTToken* token) : value(std::move(value)), LinkedType("", token) {

    }

    void link(SymbolResolver& linker, std::unique_ptr<BaseType>& current) override {
        value->link(linker, value);
        linked = value->linked_node();
    }

};