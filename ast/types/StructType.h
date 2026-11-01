// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <vector>
#include "ast/base/BaseType.h"
#include <memory>
#include "ast/structures/VariablesContainer.h"

class StructTypeCopy : public BaseType {
public:
    using BaseType::BaseType;
    BaseType* copy(ASTAllocator &allocator) override;
};

class StructType : public StructTypeCopy, public VariablesContainer {
public:

    chem::string_view name;

    /**
     * A structural tuple type `(T1, T2, ...)` is represented as an inline
     * struct whose members are named `_0`, `_1`, ... . The flag keeps its
     * source spelling and lets type facts (destructor, auto traits) walk the
     * same member list without materializing a named declaration.
     */
    bool is_tuple = false;

    /**
     * constructor
     */
    StructType(
        chem::string_view name,
        ASTNode* parent,
        SourceLocation location
    ) : VariablesContainer(ASTNodeKind::StructType, parent, location), StructTypeCopy(BaseTypeKind::Struct), name(name) {

    }

    bool equals(StructType *type);

    bool is_same(BaseType *type) final {
        return BaseType::kind() == type->kind() && equals(static_cast<StructType *>(type));
    }

    bool satisfies(BaseType *type) final;

    uint64_t byte_size(const TargetData& target) final {
        return total_byte_size(target);
    }

    ASTNode* linked_node() override {
        return this;
    }


};