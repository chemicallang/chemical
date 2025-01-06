// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"

/**
 * struct value initializer is an initializer that is inside a struct value
 * struct X { member : value } <--- member : value is the StructValueInitializer
 */
class StructMemberInitializer {
public:

    /**
     * the name of the struct member being initialized
     */
    chem::string_view name;

    /**
     * the value used to initialize
     */
    Value* value;

    /**
     * constructor
     */
    StructMemberInitializer(
        chem::string_view name,
        Value* value
    ) : name(name), value(value) {

    }

};