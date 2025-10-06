// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"
#include "ast/statements/SwitchStatement.h"

class SwitchValue : public Value {
public:

    SwitchStatement stmt;

    /**
     * constructor
     */
    SwitchValue(
        Value* expression,
        ASTNode* parent_node,
        SourceLocation loc
    ) : Value(ValueKind::SwitchValue, loc), stmt(expression, parent_node, loc) {

    }

    Value* copy(ASTAllocator &allocator) override {
        const auto copied = new (allocator.allocate<SwitchValue>()) SwitchValue(
                stmt.expression->copy(allocator),
                stmt.parent(),
                encoded_location()
        );
        stmt.copy_into(allocator, &stmt);
        return copied;
    }

    ASTNode* linked_node() override;

    Value* evaluated_value(InterpretScope &scope) override;

#ifdef COMPILER_BUILD

    static llvm::Value* llvm_value(Codegen& gen, SwitchStatement& stmt, bool allocate);

    llvm::Type* llvm_type(Codegen &gen) final;

    llvm::AllocaInst* llvm_allocate(Codegen &gen, const std::string &identifier, BaseType *expected_type) override {
        return (llvm::AllocaInst*) llvm_value(gen, stmt, true);
    }

    inline llvm::Value* llvm_value(Codegen &gen, BaseType *exp_type = nullptr) final {
        return llvm_value(gen, stmt, false);
    }

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const chem::string_view &name) override {
        const auto linked = linked_node();
        return linked != nullptr && linked->add_child_index(gen, indexes, name);
    }

#endif

};