// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BatchAllocator.h"
#include "std/chem_string_view.h"

class AllocatorStrBuilder {
public:

    BatchAllocator& allocator;
    char* data;
    std::size_t length;
    std::size_t capacity;

    AllocatorStrBuilder(
            char value,
            BatchAllocator& allocator
    ) : allocator(allocator), data(allocator.object_heap_pointer(3, 1)), length(1), capacity(3) {
        *data = value;
        *(data + 1) = '\0';
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
        return data;
    }

    void deallocate() {
        adjust_ptr(data, capacity, 0);
    }

    [[nodiscard]] chem::string_view current_view() const {
        return { data, length };
    }

    chem::string_view finalize_view() {
        return { finalize(), length };
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

/**
 * it's supposed to be used in a single threaded context
 * this supports serial allocations which means one allocation occurs
 * and then that can only be reallocated or resized and when a new allocation occurs
 * the previous is finalized and cannot change
 */
class MultiStrAllocator : public BatchAllocator {
public:

    /**
     * constructor
     */
    MultiStrAllocator(std::size_t heapBatchSize) : BatchAllocator(nullptr, 0, heapBatchSize) {
        if(heapBatchSize > 0) {
            // reserving a single heap batch size allocation for usage
            // otherwise first allocation will fail
            reserve_heap_storage();
        }
    }

    /**
     * the char will be stored in a c string
     */
    char* char_to_c_str(char c) {
        auto ptr = object_heap_pointer(sizeof(char) * 2, alignof(char));
        *ptr = c;
        *(ptr + 1) = '\0';
        return ptr;
    }

    /**
     * convert two characters to a c string
     */
    char* two_chars_to_c_str(char c1, char c2) {
        auto ptr = object_heap_pointer(sizeof(char) * 3, alignof(char));
        *ptr = c1;
        *(ptr + 1) = c2;
        *(ptr + 2) = '\0';
        return ptr;
    }


};