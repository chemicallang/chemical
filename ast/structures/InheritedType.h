// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/AccessSpecifier.h"

class InheritedType {
public:

    AccessSpecifier specifier;
    BaseType* type;

    /**
     * constructor
     */
    constexpr InheritedType(
        BaseType* type,
        AccessSpecifier specifier = AccessSpecifier::Public
    ) : type(type), specifier(specifier) {

    }

    /**
     * the name of the referenced type
     */
    chem::string_view ref_type_name();

    /**
     * make a copy of this type
     */
    [[nodiscard]]
    InheritedType copy(ASTAllocator& allocator) const;

};