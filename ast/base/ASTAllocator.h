// Copyright (c) Qinetik 2024.

#pragma once

#include "ASTAny.h"

namespace std {
    class mutex;
}

/**
 * ASTAllocator is supposed to be the simplest class that allows
 * to allocate different AST classes, it allocates a pre-allocated size on
 * stack for faster allocation
 * It always manages memory for you, meaning when it dies, all the memory it allocated dies !
 */
class ASTAllocator {
public:

    /**
     * constructor
     */
    ASTAllocator(
        char* stackMemory,
        std::size_t stackSize,
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
    inline T* allocate() {
        static_assert(std::is_base_of<ASTAny, T>::value, "T must derived from ASTAny");
        return (T*) (void*) allocate_size(sizeof(T), alignof(T));
    }

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
     * the stack memory used to store objects on stack instead of heap
     * given by the user
     */
    char* stack_memory;
    /**
     * the stack size is given by the user
     */
    std::size_t stack_memory_size;
    /**
     * how much of the stack memory has been consumed
     * by default initialized to zero
     */
    std::size_t stack_offset;

    /**
     * heap memory is a vector of bytes (multiple), with the vector we batch heap allocations
     * after initialize StackSize allocated on stack ends, we allocate another StackSize memory
     * but on heap, and we keep using it for objects until that ends, we allocate another StackSize
     * memory, this way we batch heap calls
     */
    std::vector<char*> heap_memory;
    /**
     * heap batch size is the memory allocated on the heap when stack memory ends
     */
    std::size_t heap_batch_size;

    /**
     * current heap offset
     */
    std::size_t heap_offset;

    /**
     * the mutex used to ensure memory safety when using multiple threads
     */
    std::mutex* allocator_mutex;

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
     * clear this pointer storage, only a single
     */
    void clear_ptr_storage();

    /**
     * does what it says
     */
    void destroy_memory();

    /**
     * will store the given pointer to destruct later
     */
    inline void store_ptr(char* ptr) {
        get_ptr_storage().emplace_back((ASTAny*) (void*) ptr);
    }

    /**
     * reserve a large heap storage of size (StackSize)
     * and reset current heap pointer and offset
     */
    char* reserve_heap_storage();

    /**
     * helper function to get moved heap pointer
     */
    char* offset_heap(char* const heap_ptr, std::size_t obj_size, std::size_t alignment);

    /**
     * provides a pointer for the given obj size, increments heap_current
     */
    char* object_heap_pointer(std::size_t obj_size, std::size_t alignment);

    /**
     * allocate a pointer with given object size
     */
    char* allocate_size(std::size_t obj_size, std::size_t alignment);

};