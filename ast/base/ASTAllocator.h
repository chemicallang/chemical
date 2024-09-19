// Copyright (c) Qinetik 2024.

#include "ASTAny.h"

#pragma once

/**
 * ASTAllocator is supposed to be the simplest class that allows
 * to allocate different AST classes, it allocates a pre-allocated size on
 * stack for faster allocation
 * It always manages memory for you, meaning when it dies, all the memory it allocated dies !
 * It's also very HEAVY, it's supposed to handle large amount of classes, so it allocates 1 mb on the stack
 */
template<std::size_t StackSize = 1000000>
class ASTAllocator {
public:

    /**
     * everything allocated on stack is stored on this vector of vectors
     * every pointer is just destructed, NOT freed
     */
    std::vector<std::vector<ASTAny*>> stack_allocated;
    /**
     * everything allocated on heap is stored on this vector of vectors
     * every pointer is destructed and also freed !
     */
    std::vector<std::vector<ASTAny*>> heap_allocated;

    /**
     * the stack memory used to store objects on stack instead of heap
     */
    char stackMemory[StackSize];
    /**
     * how much of the stack memory has been consumed
     * by default initialized to zero
     */
    std::size_t stack_current;


    /**
     * constructor
     */
    ASTAllocator() : stack_current(0) {
        stack_allocated.reserve(10);
        heap_allocated.reserve(10);
        reserve_another(stack_allocated);
        reserve_another(heap_allocated);
    }

    template<typename T>
    inline T* allocate() {
        static_assert(std::is_base_of<ASTAny, T>::value, "T must derived from ASTAny");
        return static_cast<T*>(allocate_size(sizeof(T)));
    }

    ~ASTAllocator() {
        for (auto& vec: stack_allocated) {
            for (auto& ptr: vec) {
                ptr->~ASTAny();
            }
        }
        for (auto& vec: heap_allocated) {
            for (auto& ptr: vec) {
                delete ptr;
            }
        }
    }

protected:

    static constexpr unsigned int PTR_VEC_SIZE = 1000;

    std::vector<ASTAny*>& reserve_another(std::vector<std::vector<ASTAny*>>& storage) {
        storage.emplace_back();
        auto& last = storage.back();
        last.reserve(PTR_VEC_SIZE);
        return last;
    }

    std::vector<ASTAny*>& ptr_storage(std::vector<std::vector<ASTAny*>>& storage) {
        auto& last = storage.back();
        if (last.size() < PTR_VEC_SIZE) {
            return last;
        } else {
            return reserve_another(storage);
        }
    }

    inline void store_ptr(std::vector<std::vector<ASTAny*>>& storage, ASTAny* ptr) {
        ptr_storage(storage).emplace_back(ptr);
    }

    void* allocate_size(std::size_t obj_size) {
        if (stack_current + obj_size < StackSize) {
            const auto ptr = stackMemory[stack_current];
            stack_current += obj_size;
            store_ptr(stack_allocated, ptr);
            return ptr;
        } else {
            const auto ptr = static_cast<void*>(::operator new(obj_size));
            store_ptr(heap_allocated, ptr);
            return ptr;
        }
    }

};