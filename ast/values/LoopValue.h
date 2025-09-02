// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"
#include "ast/structures/LoopBlock.h"

class LoopValue : public Value {
public:

    LoopBlock stmt;

    /**
     * constructor
     */
    LoopValue(
            ASTNode* parent_node,
            SourceLocation loc
    ) : Value(ValueKind::LoopValue, loc), stmt(parent_node, loc) {

    }

    Value* copy(ASTAllocator &allocator) override {
        const auto copied = new (allocator.allocate<LoopValue>()) LoopValue(
                stmt.parent(),
                encoded_location()
        );
        stmt.copy_into(allocator, &stmt);
        return copied;
    }

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen &gen) final;

    llvm::Value* llvm_value(Codegen &gen, BaseType *type = nullptr) final;

    void llvm_assign_value(Codegen &gen, llvm::Value *lhsPtr, Value *lhs) final;

    llvm::AllocaInst* llvm_allocate(Codegen &gen, const std::string &identifier, BaseType *expected_type) final;

#endif

};