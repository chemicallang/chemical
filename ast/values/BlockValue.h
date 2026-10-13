// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"
#include "ast/structures/Scope.h"

class BlockValue : public Value {

    /**
     * this is calculated during symbol resolution
     */
    Value* calculated_value = nullptr;

public:

    Scope scope;


    /**
     * constructor
     */
    constexpr BlockValue(
            ASTNode* parent_node,
            SourceLocation location
    ) : Value(ValueKind::BlockValue, location), scope(parent_node, location) {

    }

    /**
     * constructor
     */
    BlockValue(Scope scope, BaseType* type) : Value(ValueKind::BlockValue, type, scope.encoded_location()), scope(std::move(scope)) {

    }

    inline Value* getCalculatedValue() {
        return calculated_value;
    }

    void setCalculatedValue(Value* value) {
        calculated_value = value;
        if(value) {
            setType(value->getType());
        }
    }

    Value* copy(ASTAllocator &allocator) override {
        const auto blockVal = new (allocator.allocate<BlockValue>()) BlockValue(Scope{scope.parent(), scope.encoded_location()}, getType());
        scope.copy_into(blockVal->scope, allocator, scope.parent());
        blockVal->calculated_value = calculated_value;
        return blockVal;
    }



};