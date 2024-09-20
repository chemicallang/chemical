// Copyright (c) Qinetik 2024.

#include "ASTAny.h"
#include <mutex>

#pragma once

/**
 * ASTAllocator is supposed to be the simplest class that allows
 * to allocate different AST classes, it allocates a pre-allocated size on
 * stack for faster allocation
 * It always manages memory for you, meaning when it dies, all the memory it allocated dies !
 */
class ASTAllocator {
public:

    /**
     * pointers to objects user wanted are stored on this vector
     * so we can destruct them, when this allocator dies
     */
    std::vector<std::vector<ASTAny*>> ptr_storage;

    /**
     * the stack memory used to store objects on stack instead of heap
     * given by the user
     */
    char* const stack_memory;
    /**
     * the stack size is given by the user
     */
    const std::size_t stack_memory_size;
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
    const std::size_t heap_batch_size;
    /**
     * current heap offset
     */
    std::size_t heap_offset;

    /**
     * the mutex used to ensure memory safety when using multiple threads
     */
    std::mutex allocator_mutex;

    /**
     * constructor
     */
    ASTAllocator(char* stackMemory, std::size_t stackSize, std::size_t heapBatchSize) : stack_memory(stackMemory), stack_memory_size(stackSize), stack_offset(0), heap_offset(heapBatchSize), heap_batch_size(heapBatchSize) {
        ptr_storage.reserve(10);
        reserve_ptr_storage();
    }

    template<typename T>
    inline T* allocate() {
        static_assert(std::is_base_of<ASTAny, T>::value, "T must derived from ASTAny");
        return (T*) (void*) allocate_size(sizeof(T));
    }

    /**
     * when called, will free everything, and make this allocator available
     * for more allocations, basically reusing previously allocated memory
     */
    void clear() {
        std::lock_guard<std::mutex> lock(allocator_mutex);
        destroy_memory();
        clear_ptr_storage();
        stack_offset = 0;
        heap_memory.clear();
        heap_offset = heap_batch_size; // force heap allocation
    }

    /**
     * destructor
     */
    ~ASTAllocator() {
        destroy_memory();
    }

protected:

    static constexpr unsigned int PTR_VEC_SIZE = 1000;

    /**
     * create another vector reserving PTR_VEC_SIZE (1000) pointers
     */
    std::vector<ASTAny*>& reserve_ptr_storage() {
        ptr_storage.emplace_back();
        auto& last = ptr_storage.back();
        last.reserve(PTR_VEC_SIZE);
        return last;
    }

    /**
     * if there's size available in the last storage return it
     * otherwise reserve another and return that
     */
    std::vector<ASTAny*>& get_ptr_storage() {
        auto& last = ptr_storage.back();
        if (last.size() < PTR_VEC_SIZE) {
            return last;
        } else {
            return reserve_ptr_storage();
        }
    }

    /**
     * clear this pointer storage, only a single
     */
    void clear_ptr_storage() {
        if(ptr_storage.empty()) {
            reserve_ptr_storage();
        } else {
            while (ptr_storage.size() != 1) {
                ptr_storage.pop_back();
            }
            ptr_storage.back().clear();
        }
    }

    /**
     * does what it says
     */
    void destroy_memory() {
        for (auto& vec: ptr_storage) {
            for (auto& ptr: vec) {
                ptr->~ASTAny();
            }
        }
        for(auto& heap_ptr : heap_memory) {
            ::operator delete(heap_ptr);
        }
    }

    /**
     * will store the given pointer to destruct later
     */
    inline void store_ptr(ASTAny* ptr) {
        get_ptr_storage().emplace_back(ptr);
    }

    /**
     * reserve a large heap storage of size (StackSize)
     * and reset current heap pointer and offset
     */
    char* reserve_heap_storage() {
        // reserving a heap pointer with stack size
        const auto heap_pointer = static_cast<char*>(::operator new(heap_batch_size));
        heap_memory.emplace_back(heap_pointer);
        // resetting heap storage
        heap_offset = 0;
        // return
        return heap_pointer;
    }

    /**
     * provides a pointer for the given obj size, increments heap_current
     */
    char* object_heap_pointer(std::size_t obj_size) {
        const auto heap_ptr = ((heap_offset + obj_size) < heap_batch_size) ? heap_memory.back() : reserve_heap_storage();
        const auto ptr = heap_ptr + heap_offset;
        heap_offset = heap_offset + obj_size;
        return ptr;
    }

    char* allocate_size(std::size_t obj_size) {
        std::lock_guard<std::mutex> lock(allocator_mutex);
        if (stack_offset + obj_size < stack_memory_size) {
            const auto ptr = stack_memory + stack_offset;
            stack_offset += obj_size;
            store_ptr((ASTAny*) (void*) ptr);
            return ptr;
        } else {
            const auto ptr = object_heap_pointer(obj_size);
            store_ptr((ASTAny*) (void*) ptr);
            return ptr;
        }
    }

};