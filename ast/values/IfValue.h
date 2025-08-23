// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"
#include "ast/structures/If.h"

class IfValue : public Value {
public:

    IfStatement stmt;

    /**
     * constructor
     */
    IfValue(
        Value* condition,
        ASTNode* parent_node,
        SourceLocation loc
    ) : Value(ValueKind::IfValue, loc), stmt(condition, parent_node, loc) {

    }

    Value* copy(ASTAllocator &allocator) override {
        const auto copied = new (allocator.allocate<IfValue>()) IfValue(
                stmt.condition->copy(allocator),
                stmt.parent(),
                encoded_location()
        );
        stmt.copy_into(allocator, &stmt);
        return copied;
    }

    bool compile_time_computable() override {
        return stmt.compile_time_computable();
    }

    BaseType* create_type(ASTAllocator &allocator) override;

    ASTNode* linked_node() override;

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen &gen) final;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const chem::string_view &name) override;

    llvm::AllocaInst* llvm_allocate(Codegen &gen, const std::string &identifier, BaseType *expected_type) final;

    void llvm_assign_value(Codegen &gen, llvm::Value *lhsPtr, Value *lhs) final;

    llvm::Value* llvm_value(Codegen &gen, BaseType *type = nullptr) final;

#endif

};