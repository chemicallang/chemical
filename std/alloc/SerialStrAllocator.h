// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BatchAllocator.h"
#include "std/chem_string_view.h"

/**
 * it's supposed to be used in a single threaded context
 * this supports serial allocations which means one allocation occurs
 * and then that can only be reallocated or resized and when a new allocation occurs
 * the previous is finalized and cannot change
 */
class SerialStrAllocator {
public:

    /**
     * the underlying batch allocator
     */
    BatchAllocator allocator;
    char* data;
    std::size_t length;
    std::size_t capacity;

    /**
     * constructor
     */
    SerialStrAllocator(std::size_t heapBatchSize) : allocator(heapBatchSize),
        data(heapBatchSize > 0 ? allocator.object_heap_pointer(3, 1) : nullptr),
        length(0), capacity(3)
    {
        *data = '\0';
    }

    void store_heap_ptr(char* ptr) {
        auto& heap_memory = allocator.heap_memory;
        const auto current = heap_memory.back();
        heap_memory.pop_back();
        // we keep this above current, because heap_offset points to the last pointer which is current
        heap_memory.emplace_back(ptr);
        heap_memory.emplace_back(current);
    }

    void restore_heap_ptr(char* ptr) {
        auto& heap_memory = allocator.heap_memory;
        const auto current = heap_memory.back();
        heap_memory.pop_back();
        // the last time we stored the heap ptr
        heap_memory.pop_back();
        // we keep this above current, because heap_offset points to the last pointer which is current
        heap_memory.emplace_back(ptr);
        heap_memory.emplace_back(current);
    }

    void adjust_ptr(char*& ptr, const size_t old_size, const size_t new_size) {
        if(old_size == new_size) {
            return;
        } else if(new_size < old_size) {
            // previous size is within batch size
            if(old_size < allocator.heap_batch_size) {
                // we adjust the allocated offset
                allocator.heap_offset -= (old_size - new_size);
                return;
            } else {
                return;
            }
        } else {
            // new size is within batch size
            if ((allocator.heap_offset + new_size) < allocator.heap_batch_size) {
                // we adjust the allocated offset
                allocator.heap_offset += (new_size - old_size);
                return;
            } else {
                // new size within heap batch size, so we allocate a new batch instead
                if(new_size < allocator.heap_batch_size) {
                    const auto heap_batch = allocator.reserve_heap_storage();
                    allocator.heap_offset = new_size;
                    memcpy(heap_batch, ptr, old_size);
                    ptr = heap_batch;
                    return;
                }
                // if the pointer is being shifted from batch to being free
                // we de-allocate previous size of the pointer from batch and malloc it
                if(old_size < allocator.heap_batch_size && new_size >= allocator.heap_batch_size) {
                    allocator.heap_offset -= old_size;
                    const auto heap_ptr = static_cast<char*>(malloc(new_size));
                    memcpy(heap_ptr, ptr, old_size);
                    store_heap_ptr(heap_ptr);
                    ptr = heap_ptr;
                    return;
                }
                if(old_size >= allocator.heap_batch_size) {
                    // previous pointer was not within batch size, we allocated it on heap, will realloc it
                    const auto n = static_cast<char*>(realloc(ptr, new_size));
                    restore_heap_ptr(n);
                    ptr = n;
                    return;
                }
#ifdef DEBUG
                throw std::runtime_error("unknown case in serial str allocator");
#endif
            }
        }
    }

    void resize(const size_t new_capacity){
        adjust_ptr(data, capacity, new_capacity);
        capacity = new_capacity;
    }

private:

    void consume(const size_t size) {
#ifdef DEBUG
        if(capacity < size) {
            // this will only happen, if logic of append functions is incorrect
            throw std::runtime_error("cannot consume when capacity is smaller than size");
        }
#endif
        data = data + size;
        capacity = capacity - size;
        length = 0;
    }

public:

    char* finalize() {
        // save the string (char pointer) before changing it
        const auto d = data;
        // consume length of the current string (data) including last \0
        consume(length + 1);
        // return the saved string (char pointer)
        return d;
    }

    void deallocate() {
        length = 0;
        *data = '\0';
    }

    [[nodiscard]] chem::string_view current_view() const {
        return { data, length };
    }

    chem::string_view finalize_view() {
        // save the string (char pointer) & length before changing it
        const auto d = data;
        const auto l = length;
        // length + 1 because length doesn't include the last \0
        consume(length + 1);
        return { d, l };
    }

    void append(char value) {
        if(capacity <= (length + 2)){
            if((capacity * 2) <= length + 2) {
                resize(length + 2);
            } else {
                resize(capacity * 2);
            }
        }
        data[length] = value;
        data[(length + 1)] = '\0';
        length = (length + 1);
    }

};