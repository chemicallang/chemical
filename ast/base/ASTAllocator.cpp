// Copyright (c) Qinetik 2024.

#include "ASTAllocator.h"
#include <mutex>

ASTAllocator::ASTAllocator(
        char* stackMemory,
        std::size_t stackSize,
        std::size_t heapBatchSize
) : stack_memory(stackMemory), stack_memory_size(stackSize), stack_offset(0),
    heap_offset(heapBatchSize), heap_batch_size(heapBatchSize)
{
    ptr_storage.reserve(10);
    reserve_ptr_storage();
    allocator_mutex = new std::mutex;
}

void ASTAllocator::clear() {
    std::lock_guard<std::mutex> lock(*allocator_mutex);
    destroy_memory();
    clear_ptr_storage();
    stack_offset = 0;
    heap_memory.clear();
    heap_offset = heap_batch_size; // force heap allocation
}

ASTAllocator::~ASTAllocator() {
    destroy_memory();
    delete allocator_mutex;
}


std::vector<ASTAny*>& ASTAllocator::reserve_ptr_storage() {
    ptr_storage.emplace_back();
    auto& last = ptr_storage.back();
    last.reserve(PTR_VEC_SIZE);
    return last;
}

std::vector<ASTAny*>& ASTAllocator::get_ptr_storage() {
    auto& last = ptr_storage.back();
    if (last.size() < PTR_VEC_SIZE) {
        return last;
    } else {
        return reserve_ptr_storage();
    }
}

void ASTAllocator::clear_ptr_storage() {
    if (ptr_storage.empty()) {
        reserve_ptr_storage();
    } else {
        while (ptr_storage.size() != 1) {
            ptr_storage.pop_back();
        }
        ptr_storage.back().clear();
    }
}

void ASTAllocator::destroy_memory() {
    for (auto& vec: ptr_storage) {
        for (auto& ptr: vec) {
            ptr->~ASTAny();
        }
    }
    for (auto& heap_ptr: heap_memory) {
        ::operator delete(heap_ptr);
    }
}

char* ASTAllocator::reserve_heap_storage() {
    // reserving a heap pointer with stack size
    const auto heap_pointer = static_cast<char*>(::operator new(heap_batch_size));
    heap_memory.emplace_back(heap_pointer);
    // resetting heap storage
    heap_offset = 0;
    // return
    return heap_pointer;
}

char* ASTAllocator::offset_heap(char* const heap_ptr, std::size_t obj_size) {
    const auto ptr = heap_ptr + heap_offset;
    heap_offset = heap_offset + obj_size;
    return ptr;
}

char* ASTAllocator::object_heap_pointer(std::size_t obj_size) {
    if((heap_offset + obj_size) < heap_batch_size) {
        return offset_heap(heap_memory.back(), obj_size);
    } else {
        if(obj_size < heap_batch_size) {
            return offset_heap(reserve_heap_storage(), obj_size);
        } else {
            // just allocate the entire object on heap
            // and do not move heap pointer
            return static_cast<char*>(::operator new(obj_size));
        }
    }
}

char* ASTAllocator::allocate_size(std::size_t obj_size) {
    std::lock_guard<std::mutex> lock(*allocator_mutex);
    if (stack_offset + obj_size < stack_memory_size) {
        const auto ptr = stack_memory + stack_offset;
        stack_offset += obj_size;
        store_ptr(ptr);
        return ptr;
    } else {
        const auto ptr = object_heap_pointer(obj_size);
        store_ptr(ptr);
        return ptr;
    }
}