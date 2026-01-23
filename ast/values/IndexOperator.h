// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 29/02/2024.
//

#pragma once

#include <memory>
#include <utility>
#include "ast/base/Value.h"

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

    Value* evaluated_value(InterpretScope &scope) override;

    ASTNode *linked_node() final;

    bool primitive() final {
        return false;
    }

    Value *find_in(InterpretScope &scope, Value *parent) final;

    IndexOperator* copy(ASTAllocator& allocator) final {
        auto op = new (allocator.allocate<IndexOperator>()) IndexOperator((Value*) parent_val->copy(allocator), getType(), encoded_location());
        op->idx = idx->copy(allocator);
        return op;
    }

    void determine_type(TypeBuilder& typeBuilder, ASTDiagnoser& diagnoser);

#ifdef COMPILER_BUILD

    llvm::Value *elem_pointer(Codegen &gen, llvm::Type *, llvm::Value *ptr);

    llvm::Value *llvm_pointer(Codegen &gen) final;

    inline llvm::Value* loadable_llvm_pointer(Codegen& gen, SourceLocation location) {
        // overloaded operator returns reference
        // by default its a struct gep, so always requires loading
        return llvm_pointer(gen);
    }

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<Value*> &chain, unsigned int index) final;

    bool add_member_index(Codegen &gen, Value *parent, std::vector<llvm::Value *> &indexes) final;

#endif

};