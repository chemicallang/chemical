// Copyright (c) Qinetik 2024.

#pragma once

#include "ASTAny.h"
#include "utils/inline_attr.h"
#include "BatchAllocator.h"

namespace std {
    class mutex;
}

/**
 * ASTAllocator is supposed to be the simplest class that allows
 * to allocate different AST classes, It stores pointers to the allocated
 * memory to auto destruct them when the allocator dies, when the allocator
 * dies, everything allocated with it dies automatically
 */
class ASTAllocator final : public BatchAllocator {
public:

    /**
     * constructor
     */
    ASTAllocator(
        std::size_t heapBatchSize
    );

    /**
     * move constructor
     */
    ASTAllocator(
        ASTAllocator&& other
    ) noexcept;

    /**
     * move assignment
     */
    ASTAllocator& operator =(ASTAllocator&& other) noexcept;

    /**
     * the method to use to allocate a type
     */
    template<typename T>
    FORCE_INLINE T* allocate() {
        static_assert(std::is_base_of<ASTAny, T>::value, "T must derived from ASTAny");
        return (T*) (void*) allocate_size(sizeof(T), alignof(T));
    }

    /**
     * allocate a pointer with given object size and store the pointer
     */
    char* allocate_size(std::size_t obj_size, std::size_t alignment);

    /**
     * when called, will free everything, and make this allocator available
     * for more allocations, basically reusing previously allocated memory
     */
    void clear();

    /**
      * destructor
      */
    ~ASTAllocator();

protected:

    /**
     * pointers to objects user wanted are stored on this vector
     * so we can destruct them, when this allocator dies
     */
    std::vector<std::vector<ASTAny*>> ptr_storage;

    /**
     * pointer vector size, default 1000 pointers are allocated
     */
    static constexpr unsigned int PTR_VEC_SIZE = 1000;

    /**
     * create another vector reserving PTR_VEC_SIZE (1000) pointers
     */
    std::vector<ASTAny*>& reserve_ptr_storage();

    /**
     * if there's size available in the last storage return it
     * otherwise reserve another and return that
     */
    std::vector<ASTAny*>& get_ptr_storage();

    /**
     * destructs the pointer storage
     */
    void destruct_ptr_storage();

    /**
     * clear this pointer storage, only a single
     */
    void clear_ptr_storage();

    /**
     * will store the given pointer to destruct later
     */
    inline void store_ptr(char* ptr) {
        get_ptr_storage().emplace_back((ASTAny*) (void*) ptr);
    }

};