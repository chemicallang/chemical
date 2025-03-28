// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"

/**
 * a parameter is added to the function implicitly when it returns a struct
 * when user returns a struct the returning struct is copied into this implicitly
 * passed struct, with this value user can gain access to implicitly passed struct
 * and then modify that
 */
class RetStructParamValue : public Value {
public:

    /**
     * constructor
     */
    explicit constexpr RetStructParamValue(SourceLocation location) : Value(ValueKind::RetStructParamValue, location) {

    }

#ifdef COMPILER_BUILD

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

#endif

    BaseType* create_type(ASTAllocator& allocator) final;

    Value *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<RetStructParamValue>()) RetStructParamValue(encoded_location());
    }

};