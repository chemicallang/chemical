// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTAllocator.h"

class TypeBuilder;

class ASTBuilder {
public:

    ASTAllocator* allocator;

    TypeBuilder& typeBuilder;

    /**
     * the method to use to allocate a type
     */
    template<typename T>
    FORCE_INLINE T* allocate() {
        static_assert(std::is_base_of<ASTAny, T>::value, "T must derived from ASTAny");
        return (T*) (void*) allocator->allocate_size(sizeof(T), alignof(T));
    }



};