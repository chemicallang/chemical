// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/LoopASTNode.h"
#include "ast/base/Value.h"
#include "Scope.h"
#include "ast/statements/VarInit.h"
#include "ast/types/ReferenceType.h"
#include <cstdint>
#include <string>

struct ForInLoopAttributes {

    bool reversed = false;

    bool reversed_counter = false;

    bool is_reference = false;

    bool stoppedInterpretation = false;

};

enum class ForInLoopIterationKind : uint8_t {
    Unknown,
    Array,
    Linear,
    Chunked,
    Iterable
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
    ForInLoopIterationKind iteration_kind = ForInLoopIterationKind::Unknown;

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

    bool is_reference() const noexcept {
        return attrs.is_reference;
    }

    void set_is_reference(bool value) noexcept {
        attrs.is_reference = value;
    }

    bool is_reference_mutable() const noexcept {
        // when is_reference is true, we attach a reference type to elem_type, which contains whether its mutable
        return is_reference() && elem_type->as_reference_type_unsafe()->is_mutable;
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
        loop->iteration_kind = iteration_kind;
        body.copy_into(loop->body, allocator, loop);
        return loop;
    }

    BaseType* getIterationElementActualType() const noexcept {
        return is_reference() ? elem_type->as_reference_type_unsafe()->type : elem_type;
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

    bool add_child_index(Codegen& gen, std::vector<llvm::Value*>& indexes, const chem::string_view& name) override;

#endif

    void stopInterpretation() final;

};
