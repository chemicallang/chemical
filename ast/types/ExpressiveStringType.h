// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"

class ExpressiveStringType : public GlobalBaseType {
public:

    ExpressiveStringType() : GlobalBaseType(BaseTypeKind::ExpressiveString) {

    }

    /**
     * A `%expressive_string` parameter is a formatting parameter: it takes a
     * backtick template or a string literal (whose type is `String`), and
     * lowers it through `intrinsics::expr_str_block_value`. This is what lets
     * `println("literal")` and `env.error(`the int ${x} is wrong`)` take the
     * value directly instead of requiring a `std::string` wrapper.
     *
     * Only these two kinds are accepted at the type level so this parameter
     * cannot shadow a precise constructor overload (`string(value : ...)`) that
     * it is often declared next to.
     */
    bool satisfies(BaseType* type) override {
        const auto kind = type->kind();
        return kind == BaseTypeKind::ExpressiveString || kind == BaseTypeKind::String;
    }

};
