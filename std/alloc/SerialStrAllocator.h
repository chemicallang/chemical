// Copyright (c) Qinetik 2024.

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
    SerialStrAllocator(std::size_t heapBatchSize) : allocator(heapBatchSize), data(heapBatchSize > 0 ? allocator.object_heap_pointer(3, 1) : nullptr),
        length(0), capacity(3) {}

    void init_new() {
        data = allocator.object_heap_pointer(3, 1);
        length = 0;
        capacity = 3;
    }

    void adjust_ptr(char*& ptr, const size_t old_size, const size_t new_size) {
        if(old_size == new_size) {
            return;
        } else if(new_size < old_size) {
            allocator.heap_offset -= (old_size - new_size);
            return;
        } else {
            if ((allocator.heap_offset + new_size) < allocator.heap_batch_size) {
                allocator.heap_offset += (new_size - old_size);
                return;
            } else {
                auto new_ptr = allocator.object_heap_pointer(new_size, 1);
                memcpy(new_ptr, ptr, old_size);
                ptr = new_ptr;
            }
        }
    }

    void resize(const size_t new_capacity){
        adjust_ptr(data, capacity, new_capacity);
        capacity = new_capacity;
    }

    char* finalize() {
        // length + 1 because length doesn't include the last \0
        adjust_ptr(data, capacity, length + 1);
        const auto d = data;
        init_new();
        return d;
    }

    void deallocate() {
        adjust_ptr(data, capacity, 0);
        init_new();
    }

    [[nodiscard]] chem::string_view current_view() const {
        return { data, length };
    }

    chem::string_view finalize_view() {
        // length + 1 because length doesn't include the last \0
        adjust_ptr(data, capacity, length + 1);
        const auto d = data;
        const auto l = length;
        init_new();
        return { d, l };
    }

    void append(char value) {
        if((capacity <= (length + 2))){
            resize((capacity * 2));
        }
        data[length] = value;
        data[(length + 1)] = '\0';
        length = (length + 1);
    }

};