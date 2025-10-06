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

    ASTNode* linked_node() override;

    Value* evaluated_value(InterpretScope &scope) override;

#ifdef COMPILER_BUILD

    static llvm::Value* llvm_value(Codegen& gen, IfStatement& stmt, bool allocate);

    llvm::Type* llvm_type(Codegen &gen) final;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const chem::string_view &name) override;

    llvm::AllocaInst* llvm_allocate(Codegen &gen, const std::string &identifier, BaseType *expected_type) override {
        return (llvm::AllocaInst*) llvm_value(gen, stmt, true);
    }

    inline llvm::Value* llvm_value(Codegen &gen, BaseType *type = nullptr) final {
        return llvm_value(gen, stmt, false);
    }

#endif

};