// Copyright (c) Qinetik 2024.

#pragma once

#include "LinkedType.h"
#include "ast/base/Value.h"

class LinkedValueType : public LinkedType {
public:

    Value* value;

    LinkedValueType(Value* value, SourceLocation location) : value(value), LinkedType("", location) {

    }

    LinkedValueType(Value* value, chem::string_view type, ASTNode* linked, SourceLocation location) : value(value), LinkedType(type, linked, location) {

    }

    LinkedType* copy(ASTAllocator &allocator) const final {
        return new (allocator.allocate<LinkedValueType>()) LinkedValueType(
            value->copy(allocator),
            type,
            linked,
            location
        );
    }

    bool link(SymbolResolver &linker) final {
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