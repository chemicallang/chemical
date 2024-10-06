// Copyright (c) Qinetik 2024.

#pragma once

#include "LinkedType.h"
#include "ast/base/Value.h"

class LinkedValueType : public LinkedType {
public:

    Value* value;

    LinkedValueType(Value* value, CSTToken* token) : value(value), LinkedType("", token) {

    }

    LinkedType* copy(ASTAllocator &allocator) const override {
        return new (allocator.allocate<LinkedValueType>()) LinkedValueType(
            value->copy(allocator),
            token
        );
    }

    bool link(SymbolResolver &linker) override {
        if(!value->link(linker, value)) {
            return false;
        }
        linked = value->linked_node();
        if(!linked) {
            return false;
        }
        return true;
    }

};