// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"

class BoolType : public GlobalBaseType {
public:

    /**
     * constructor
     */
    constexpr BoolType() : GlobalBaseType(BaseTypeKind::Bool) {

    }

    uint64_t byte_size(const TargetData& target) final {
        return 1;
    }

    bool satisfies(BaseType *type) final;


};