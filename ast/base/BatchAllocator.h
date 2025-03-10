// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ASTAny.h"
#include "utils/inline_attr.h"

namespace std {
    class mutex;
}

/**
 * ASTAllocator is supposed to be the simplest class that allows
 * to allocate different AST classes, it allocates a pre-allocated size on
 * stack for faster allocation
 * It always manages memory for you, meaning when it dies, all the memory it allocated dies !
 */
class BatchAllocator {
public:

    friend class SerialStrAllocator;

    /**
     * constructor
     */
    BatchAllocator(std::size_t heapBatchSize);

    /**
     * move constructor
     */
    BatchAllocator(
            BatchAllocator&& other
    ) noexcept;

    /**
     * move assignment
     */
    BatchAllocator& operator =(BatchAllocator&& other) noexcept;

    /**
     * allocate a type and release it from allocator, the type must
     * be destructed by the caller, not freed, just destructed
     */
    template<typename T>
    FORCE_INLINE T* allocate_released() {
        return (T*) (void*) allocate_released_size(sizeof(T), alignof(T));
    }

    /**
     * allocate a pointer with given object size, you must destruct the allocated pointer
     * otherwise there'll be a memory leak
     */
    char* allocate_released_size(std::size_t obj_size, std::size_t alignment);

    /**
     * allocate the given string on the allocator and get a c string
     */
    char* allocate_str(const char* data, std::size_t size);

    /**
      * destructor
      */
    ~BatchAllocator();

protected:

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
     * does what it says
     */
    void destroy_memory();

    /**
     * reserve a large heap storage of size (StackSize)
     * and reset current heap pointer and offset
     */
    char* reserve_heap_storage();

    /**
     * provides a pointer for the given obj size, increments heap_current
     */
    char* object_heap_pointer(std::size_t obj_size, std::size_t alignment);

};