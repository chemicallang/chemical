// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/LoopASTNode.h"
#include "ast/base/Value.h"
#include "Scope.h"

/**
 * loop block represents the Loop Block
 *
 * loop { } <-- which repeats infinitely until stopped by
 * a break statement, the code generated by a loop block is very concise
 *
 */
class LoopBlock : public LoopASTNode, public Value {
public:

    ASTNode* parent_node;
    Value* first_broken = nullptr;
    SourceLocation location;

    /**
     * constructor
     */
    LoopBlock(Scope scope, ASTNode* parent_node, SourceLocation location) : LoopASTNode(std::move(scope)), parent_node(parent_node), location(location) {

    }

    SourceLocation encoded_location() final {
        return location;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    ASTNode* parent() final {
        return parent_node;
    }

    ASTNodeKind kind() final {
        return ASTNodeKind::LoopBlock;
    }

    ValueKind val_kind() final {
        return ValueKind::LoopValue;
    }

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

    bool link(SymbolResolver &linker, Value* &value_ptr, BaseType *expected_type = nullptr) final;

    Value* get_first_broken();

    BaseType* create_value_type(ASTAllocator& allocator) final;

    BaseType* create_type(ASTAllocator& allocator) final;

    BaseType* known_type() final;

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen &gen) final;

    llvm::Value* llvm_value(Codegen &gen, BaseType *type = nullptr) final;

    void llvm_assign_value(Codegen &gen, llvm::Value *lhsPtr, Value *lhs) final;

    llvm::AllocaInst* llvm_allocate(Codegen &gen, const std::string &identifier, BaseType *expected_type) final;

    void code_gen(Codegen &gen) final;

#endif

};