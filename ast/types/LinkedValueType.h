// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "LinkedType.h"
#include "ast/base/Value.h"

class LinkedValueType : public LinkedType {
public:

    Value* value;

    /**
     * constructor
     */
    constexpr LinkedValueType(
            Value* value
    ) : value(value), LinkedType(nullptr, false, true) {

    }

    /**
     * constructor
     */
    constexpr LinkedValueType(
            Value* value,
            ASTNode* linked
    ) : LinkedType(linked, false, true), value(value) {

    }

    LinkedType* copy(ASTAllocator &allocator) const final {
        return new (allocator.allocate<LinkedValueType>()) LinkedValueType(
            value->copy(allocator),
            linked
        );
    }

    bool link(SymbolResolver &linker, SourceLocation loc) final {
        if(!value->link(linker)) {
            return false;
        }
        linked = value->linked_node();
        if(!linked) {
            return false;
        }
        return true;
    }

};