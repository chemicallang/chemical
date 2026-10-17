// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 29/02/2024.
//

#pragma once

#include <memory>
#include <utility>
#include "ast/base/Value.h"

class CoreNodes;
class ImplementationsIndex;

class IndexOperator : public Value {
public:

    Value* parent_val;
    Value* idx;

    /**
     * constructor
     */
    constexpr IndexOperator(
            Value* parent,
            SourceLocation location
    ) : Value(ValueKind::IndexOperator, location), parent_val(parent) {

    }

    /**
     * constructor
     */
    constexpr IndexOperator(
            Value* parent,
            BaseType* type,
            SourceLocation location
    ) : Value(ValueKind::IndexOperator, type, location), parent_val(parent) {

    }


    ASTNode *linked_node() final;

    bool primitive() final {
        return false;
    }

    IndexOperator* copy(ASTAllocator& allocator) final {
        auto op = new (allocator.allocate<IndexOperator>()) IndexOperator((Value*) parent_val->copy(allocator), getType(), encoded_location());
        op->idx = idx->copy(allocator);
        return op;
    }

    void determine_type(const TypeBuilder& typeBuilder, const CoreNodes& coreNodes, const ImplementationsIndex& implsIndex, ASTDiagnoser& diagnoser);


};