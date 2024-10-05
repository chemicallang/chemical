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

ASTAllocator::ASTAllocator(
    ASTAllocator&& other
) noexcept : ptr_storage(std::move(other.ptr_storage)), stack_memory(other.stack_memory), stack_memory_size(other.stack_memory_size), stack_offset(other.stack_offset),
    heap_memory(std::move(other.heap_memory)), heap_batch_size(other.heap_batch_size), heap_offset(other.heap_offset), allocator_mutex(other.allocator_mutex)
{
    other.stack_memory = nullptr;
    other.stack_memory_size = 0;
    other.stack_offset = 0;
    other.heap_offset = 0;
    other.allocator_mutex = new std::mutex;
}

ASTAllocator& ASTAllocator::operator =(ASTAllocator&& other) noexcept {

    ptr_storage = std::move(other.ptr_storage);
    stack_memory = other.stack_memory;
    stack_memory_size = other.stack_memory_size;
    stack_offset = other.stack_offset;
    heap_memory = std::move(other.heap_memory);
    heap_batch_size = other.heap_batch_size;
    heap_offset = other.heap_offset;
    allocator_mutex = other.allocator_mutex;

    other.stack_memory = nullptr;
    other.stack_memory_size = 0;
    other.stack_offset = 0;
    other.heap_offset = 0;
    other.allocator_mutex = new std::mutex;

    return *this;
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

char* ASTAllocator::offset_heap(char* const heap_ptr, std::size_t obj_size, std::size_t alignment) {
    std::size_t aligned_offset = (heap_offset + alignment - 1) & ~(alignment - 1);
    const auto ptr = heap_ptr + aligned_offset;
    heap_offset = aligned_offset + obj_size;
    return ptr;
}

char* ASTAllocator::object_heap_pointer(std::size_t obj_size, std::size_t alignment) {
    if((heap_offset + obj_size) < heap_batch_size) {
        return offset_heap(heap_memory.back(), obj_size, alignment);
    } else {
        if(obj_size < heap_batch_size) {
            return offset_heap(reserve_heap_storage(), obj_size, alignment);
        } else {
            // just allocate the entire object on heap
            // and do not move heap pointer
            return static_cast<char*>(::operator new(obj_size, std::align_val_t(alignment)));
        }
    }
}

char* ASTAllocator::allocate_size(std::size_t obj_size, std::size_t alignment) {
    std::lock_guard<std::mutex> lock(*allocator_mutex);
    std::size_t aligned_stack_offset = (stack_offset + alignment - 1) & ~(alignment - 1);
    if (aligned_stack_offset + obj_size < stack_memory_size) {
        const auto ptr = stack_memory + aligned_stack_offset;
        stack_offset = aligned_stack_offset + obj_size;
        store_ptr(ptr);
        return ptr;
    } else {
        const auto ptr = object_heap_pointer(obj_size, alignment);
        store_ptr(ptr);
        return ptr;
    }
}