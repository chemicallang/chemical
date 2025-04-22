// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"
#include "ast/base/ASTNode.h"
#include "ast/types/PointerType.h"

/**
 * just responsible for allocating a type on the heap
 */
class NewTypedValue : public Value {
public:

    BaseType* type;
    // TODO remove this
    PointerType ptr_type;

    /**
     * constructor
     */
    inline constexpr NewTypedValue(
        BaseType* type,
        SourceLocation location
    ) : Value(ValueKind::NewTypedValue, location), type(type), ptr_type(nullptr, false) {

    }

    bool link(SymbolResolver &linker, Value *&value_ptr, BaseType *expected_type = nullptr) override;

    BaseType* create_type(ASTAllocator &allocator) override;

    BaseType* known_type() override;

    ASTNode* linked_node() override {
        return type->linked_node();
    }

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen &gen) override;

    llvm::Value* llvm_value(Codegen &gen, BaseType *type = nullptr) override;

    bool add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) override {
        const auto node = type->linked_node();
        if(!node) return false;
        return node->add_child_index(gen, indexes, name);
    }

#endif

};