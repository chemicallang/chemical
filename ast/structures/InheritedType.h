// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/AccessSpecifier.h"

class InheritedType {
public:

    AccessSpecifier specifier;
    std::unique_ptr<BaseType> type;

    explicit InheritedType(
            std::unique_ptr<BaseType> type,
            AccessSpecifier specifier = AccessSpecifier::Public
    );

    /**
     * the name of the referenced type
     */
    std::string& ref_type_name();

    /**
     * make a copy of this type
     */
    [[nodiscard]]
    InheritedType *copy() const;

};