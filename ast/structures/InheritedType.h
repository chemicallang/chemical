// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/AccessSpecifier.h"

class InheritedType {
public:

    AccessSpecifier specifier;
    BaseType* type;

    explicit InheritedType(
            BaseType* type,
            AccessSpecifier specifier = AccessSpecifier::Public
    );

    /**
     * the name of the referenced type
     */
    const chem::string_view& ref_type_name();

    /**
     * make a copy of this type
     */
    [[nodiscard]]
    InheritedType copy(ASTAllocator& allocator) const;

};