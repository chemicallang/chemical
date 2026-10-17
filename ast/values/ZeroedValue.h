// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"
#include "ast/base/TypeLoc.h"

class ZeroedValue : public Value {
public:

    bool is_unsafe = false;

    SourceLocation type_location;

    ZeroedValue(
            TypeLoc type_loc,
            SourceLocation location,
            bool is_unsafe = false
    ) : Value(ValueKind::ZeroedValue, (BaseType*) type_loc.getType(), location), is_unsafe(is_unsafe), type_location(type_loc.getLocation()) {

    }

    Value* copy(ASTAllocator &allocator) override {
        return new (allocator.allocate<ZeroedValue>()) ZeroedValue(
            { getType()->copy(allocator), type_location }, encoded_location(), is_unsafe
        );
    }


};
