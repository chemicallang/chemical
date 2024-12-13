// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BatchAllocator.h"
#include "std/chem_string_view.h"
#include "std/chem_span.h"

template<typename T>
class SerialVecBuilder {
public:

    BatchAllocator& allocator;
    T* data;
    std::size_t length;
    std::size_t capacity;

    /**
     * this allocates a single object of type T in the allocator and assigns the provided value
     * to the allocated data
     */
    SerialVecBuilder(
        T value,
        BatchAllocator& allocator
    );

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

    T* finalize();

    void deallocate() {
        adjust_ptr(data, capacity, 0);
    }

    chem::span<T> current_view() const {
        return { data, length };
    }

    chem::span<T> finalize_view() const {
        return { finalize(), length };
    }

    template<typename U = T>
    std::enable_if_t<std::is_same_v<U, char>, chem::string_view> current_view() const {
        return { data, length };
    }

    template<typename U = T>
    std::enable_if_t<std::is_same_v<U, char>, chem::string_view> finalize_view() {
        return { data, length };
    }

    template<typename U = T>
    std::enable_if_t<std::is_same_v<U, char>, void> append(char value) {
        if((capacity <= (length + 2))){
            resize((capacity * 2));
        }
        data[length] = value;
        data[(length + 1)] = '\0';
        length++;
    }

    void append(T value) {
        if(capacity <= length + 1) {
            resize(capacity * 2);
        }
        data[length] = value;
        length++;
    }


};

template<typename T>
SerialVecBuilder<T>::SerialVecBuilder(
        T value,
        BatchAllocator& allocator
) : allocator(allocator), data(allocator.object_heap_pointer(sizeof(T), alignof(T))), length(1), capacity(1) {
    *data = value;
}

/**
 * for a single character we reserve the \0 character as well, so instead of we allocate three bytes to start with
 */
 template<>
SerialVecBuilder<char>::SerialVecBuilder(
        char value,
        BatchAllocator& allocator
) : allocator(allocator), data(allocator.object_heap_pointer(3, 1)), length(1), capacity(3) {
    *data = value;
    *(data + 1) = '\0';
}

template<typename T>
T* SerialVecBuilder<T>::finalize() {
    adjust_ptr(data, capacity, length);
    return data;
}

template<>
char* SerialVecBuilder<char>::finalize() {
    // length + 1 because length doesn't include the last \0
    adjust_ptr(data, capacity, length + 1);
    return data;
}