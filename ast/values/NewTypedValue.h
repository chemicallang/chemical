// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"
#include "ast/base/ASTNode.h"
#include "ast/base/TypeLoc.h"
#include "ast/types/PointerType.h"

/**
 * just responsible for allocating a type on the heap
 */
class NewTypedValue : public Value {
public:

    // TODO remove this, we can store the user given type inside pointer type
    // write inline function to retrieve the type from pointer type
    // currently pointer type doesn't contain have location of a child type
    TypeLoc type;
    PointerType ptr_type;

    /**
     * constructor
     */
    inline constexpr NewTypedValue(
        TypeLoc type,
        SourceLocation location
    ) : Value(ValueKind::NewTypedValue, &ptr_type, location), type(type), ptr_type(type, true) {

    }

    ASTNode* linked_node() override {
        return type->linked_node();
    }

    Value* evaluated_value(InterpretScope &scope) override;


};