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

    ASTNode* linked_node() override {
        return this;
    }


};