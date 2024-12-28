// Copyright (c) Qinetik 2024.

#pragma once

#include "ASTAny.h"
#include "utils/inline_attr.h"
#include "BatchAllocator.h"

namespace std {
    class mutex;
}

struct ASTCleanupFunction {
    void* instance_ptr;
    void(*cleanup_fn)(void*);
};

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
     * the pointer is considered pointing to ASTAny object, which is virtually destructed
     * at the destruction of this ASTAllocator
     */
    char* allocate_size(std::size_t obj_size, std::size_t alignment);

    /**
     * allocates a pointer with given object size and store the cleanup function
     * the pointer is considered an instance for the given cleanup function, this cleanup
     * function will be called at destruction of this Allocator with the allocated pointer as
     * an argument
     */
    char* allocate_with_cleanup(std::size_t obj_size, std::size_t alignment, void* cleanup_fn);

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
    std::vector<ASTAny*> ptr_storage;

    /**
     * these functions are run at destruction, we can basically
     * store any object and it's destructor in this struct, and call it
     * this allows us to store chemical objects with destructors
     * of classes created by user in chemical
     */
    std::vector<ASTCleanupFunction> cleanup_fns;

    /**
     * pointer vector size, default 1000 pointers are allocated
     */
    static constexpr unsigned int PTR_VEC_SIZE = 1000;

    /**
     * destructs the pointer storage
     */
    void destruct_ptr_storage();

    /**
     * destructs clean up functions
     */
    void destruct_cleanup_storage();

    /**
     * stores the cleanup function to be called at destruction
     */
    inline void store_cleanup_fn(void* instance, void* cleanup_fn) {
        cleanup_fns.emplace_back(instance, (void(*)(void*)) cleanup_fn);
    }

    /**
     * will store the given pointer to destruct later
     */
    inline void store_ptr(char* ptr) {
        ptr_storage.emplace_back((ASTAny*) (void*) ptr);
    }

};