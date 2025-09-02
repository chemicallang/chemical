// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"
#include "ast/base/ASTNode.h"
#include "ast/base/TypeLoc.h"
#include "ast/types/PointerType.h"

/**
 * just responsible for allocating a type on the heap
 */
class NewTypedValue : public Value {
public:

    // TODO remove this, we can store the user given type inside pointer type
    // write inline function to retrieve the type from pointer type
    // currently pointer type doesn't contain have location of a child type
    TypeLoc type;
    PointerType ptr_type;

    /**
     * constructor
     */
    inline constexpr NewTypedValue(
        TypeLoc type,
        SourceLocation location
    ) : Value(ValueKind::NewTypedValue, &ptr_type, location), type(type), ptr_type(type, true) {

    }

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