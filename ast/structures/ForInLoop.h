// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/LoopASTNode.h"
#include "ast/base/Value.h"
#include "Scope.h"
#include "ast/statements/VarInit.h"

struct ForInLoopAttributes {

    bool reversed = false;

    bool reversed_counter = false;

    bool stoppedInterpretation = false;

};

class ForInLoop : public LoopASTNode {
public:

    chem::string_view id;
    VarInitStatement* index_init;
    Value* expr;
    ForInLoopAttributes attrs;
    // calculated during sym res
    // the type of element
    BaseType* elem_type = nullptr;

#ifdef COMPILER_BUILD
    llvm::Value* id_ptr = nullptr;
#endif


    /**
     * @brief Construct a new ForLoop object.
     */
    constexpr ForInLoop(
            const chem::string_view& id,
            VarInitStatement* index_init,
            Value* expr,
            ASTNode* parent_node,
            SourceLocation location
    ) : LoopASTNode(ASTNodeKind::ForInLoopStmt, parent_node, location), id(id), index_init(index_init), expr(expr) {

    }

    bool is_reversed() const noexcept {
        return attrs.reversed;
    }

    void set_reversed(bool value) noexcept {
        attrs.reversed = value;
    }

    bool is_reversed_counter() const noexcept {
        return attrs.reversed_counter;
    }

    void set_reversed_counter(bool value) noexcept {
        attrs.reversed_counter = value;
    }

    inline BaseType* known_type() const noexcept {
        return elem_type;
    }

    ForInLoop* copy(ASTAllocator &allocator) override {
        const auto copied_expr = expr->copy(allocator);
        const auto loop = new (allocator.allocate<ForInLoop>()) ForInLoop(
            id,
            // index init is copyable
            index_init,
            copied_expr,
            parent(),
            encoded_location()
        );
        loop->attrs = attrs;
        body.copy_into(loop->body, allocator, loop);
        return loop;
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final;

    llvm::Type* llvm_type(Codegen& gen) override {
        return elem_type->llvm_type(gen);
    }

    inline llvm::Value* llvm_pointer(Codegen& gen) {
        return id_ptr;
    }

    inline llvm::Value* loadable_llvm_pointer(Codegen& gen, SourceLocation location) {
        return id_ptr;
    }

    llvm::Value* llvm_load(Codegen& gen, SourceLocation location) override;

#endif

    void stopInterpretation() final;

};