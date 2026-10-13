// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"

class DoubleType : public GlobalBaseType {
public:

    /**
     * constructor
     */
    constexpr DoubleType() : GlobalBaseType(BaseTypeKind::Double) {

    }

    uint64_t byte_size(const TargetData& target) final {
        return 8;
    }

    bool satisfies(BaseType *type) final {
        return type->kind() == BaseTypeKind::Double;
    }


};