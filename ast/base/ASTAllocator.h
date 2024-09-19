// Copyright (c) Qinetik 2024.

#include "ASTAny.h"

#pragma once

/**
 * ASTAllocator is supposed to be the simplest class that allows
 * to allocate different AST classes
 */
class ASTAllocator {
public:

    bool deallocate = false;

    ASTAllocator() {

    }

    template <typename T>
    T* allocate() {
        static_assert(std::is_base_of<ASTAny, T>::value, "T must derived from ASTAny");
        return static_cast<T*>(::operator new(sizeof(T)));
    }

    template <typename T>
    T* allocate_node() {
        static_assert(std::is_base_of<ASTNode, T>::value, "T must derived from ASTNode");
        return static_cast<T*>(::operator new(sizeof(T)));
    }

    template <typename T>
    T* allocate_value() {
        static_assert(std::is_base_of<Value, T>::value, "T must derived from Value");
        return static_cast<T*>(::operator new(sizeof(T)));
    }

    template <typename T>
    T* allocate_type() {
        static_assert(std::is_base_of<BaseType, T>::value, "T must derived from Type");
        return static_cast<T*>(::operator new(sizeof(T)));
    }

};