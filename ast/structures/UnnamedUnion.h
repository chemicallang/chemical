// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "VariablesContainer.h"
#include "BaseDefMember.h"
#include "ast/types/UnionType.h"
#include "ast/types/LinkedType.h"

class UnnamedUnion : public BaseDefMember, public VariablesContainerBase {
public:

    AccessSpecifier specifier;
    LinkedType linkedType;

    /**
     * constructor
     */
    UnnamedUnion(
        chem::string_view name,
        ASTNode* parent_node,
        SourceLocation location,
        AccessSpecifier specifier = AccessSpecifier::Internal
    ) : BaseDefMember(name, ASTNodeKind::UnnamedUnion, parent_node, location), specifier(specifier), linkedType(this) {

    }

    bool get_is_const() final {
        // TODO allow user to mark unnamed struct's const
        return false;
    }

    UnnamedUnion* copy_member(ASTAllocator &allocator) final {
        auto unnamed = new (allocator.allocate<UnnamedUnion>()) UnnamedUnion(name, parent(), encoded_location());
        VariablesContainerBase::copy_direct_variables_into(*unnamed, allocator, this);
        return unnamed;
    }

    uint64_t byte_size(const TargetData& target) final {
        return largest_member()->byte_size(target);
    }

    inline BaseType* known_type() {
        return &linkedType;
    }


};