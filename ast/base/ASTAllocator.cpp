// Copyright (c) Qinetik 2024.

#include "ASTAllocator.h"
#include <mutex>
#include "cstring"

BatchAllocator::BatchAllocator(std::size_t heapBatchSize) : heap_offset(heapBatchSize), heap_batch_size(heapBatchSize) {
    if(heapBatchSize > 0) {
        // reserving a single heap batch size allocation for usage
        // otherwise first allocation will fail
        reserve_heap_storage();
    }
    allocator_mutex = new std::mutex;
}


ASTAllocator::ASTAllocator(
    std::size_t heapBatchSize
) : BatchAllocator(heapBatchSize) {
    ptr_storage.reserve(PTR_VEC_SIZE);
    cleanup_fns.reserve(PTR_VEC_SIZE);
}

BatchAllocator::BatchAllocator(
        BatchAllocator&& other
) noexcept : heap_memory(std::move(other.heap_memory)), heap_batch_size(other.heap_batch_size), heap_offset(other.heap_offset),
    allocator_mutex(other.allocator_mutex)
{
    other.heap_offset = 0;
    other.allocator_mutex = new std::mutex;
}

ASTAllocator::ASTAllocator(
    ASTAllocator&& other
) noexcept : BatchAllocator(std::move(other)), ptr_storage(std::move(other.ptr_storage)),
    cleanup_fns(std::move(other.cleanup_fns))
{
}

BatchAllocator& BatchAllocator::operator =(BatchAllocator&& other) noexcept {

    this->~BatchAllocator();

    heap_memory = std::move(other.heap_memory);
    heap_batch_size = other.heap_batch_size;
    heap_offset = other.heap_offset;
    allocator_mutex = other.allocator_mutex;

    other.heap_offset = 0;
    other.allocator_mutex = new std::mutex;

    return *this;
}

ASTAllocator& ASTAllocator::operator =(ASTAllocator&& other) noexcept {
    BatchAllocator::operator=(std::move(other));
    ptr_storage = std::move(other.ptr_storage);
    cleanup_fns = std::move(other.cleanup_fns);
    return *this;
}

void ASTAllocator::clear() {
    std::lock_guard<std::mutex> lock(*allocator_mutex);
    destruct_ptr_storage();
    destruct_cleanup_storage();
    destroy_memory();
    heap_memory.clear();
    heap_offset = heap_batch_size; // force heap allocation
}

void BatchAllocator::destroy_memory() {
    for (const auto heap_ptr: heap_memory) {
        ::operator delete(heap_ptr);
    }
}

BatchAllocator::~BatchAllocator() {
    destroy_memory();
    delete allocator_mutex;
}

void ASTAllocator::destruct_ptr_storage() {
    for (const auto ptr: ptr_storage) {
        ptr->~ASTAny();
    }
    ptr_storage.clear();
}

void ASTAllocator::destruct_cleanup_storage() {
    for(auto& ptr : cleanup_fns) {
        ptr.cleanup_fn(ptr.instance_ptr);
    }
    cleanup_fns.clear();
}

ASTAllocator::~ASTAllocator() {
    destruct_cleanup_storage();
    destruct_ptr_storage();
}

char* BatchAllocator::reserve_heap_storage() {
    // reserving a heap pointer with stack size
    const auto heap_pointer = static_cast<char*>(::operator new(heap_batch_size));
    heap_memory.emplace_back(heap_pointer);
    // resetting heap storage
    heap_offset = 0;
    // return
    return heap_pointer;
}

char* BatchAllocator::object_heap_pointer(std::size_t obj_size, std::size_t alignment) {
     std::size_t aligned_heap_offset = (heap_offset + alignment - 1) & ~(alignment - 1);
    if((aligned_heap_offset + obj_size) < heap_batch_size) {
        heap_offset = aligned_heap_offset + obj_size;
        return heap_memory.back() + aligned_heap_offset;
    } else {
        if(obj_size < heap_batch_size) {
            const auto ptr = reserve_heap_storage();
            const auto new_aligned_heap_offset = (0 + alignment - 1) & ~(alignment - 1);
            heap_offset = new_aligned_heap_offset + obj_size;
            return ptr + new_aligned_heap_offset;
        } else {
            // just allocate the entire object on heap
            // and do not move heap pointer
            const auto ptr = static_cast<char*>(::operator new(obj_size, std::align_val_t(alignment)));
            if(heap_memory.empty()) {
                heap_memory.emplace_back(ptr);
            } else {
                const auto current = heap_memory.back();
                heap_memory[heap_memory.size() - 1] = ptr;
                heap_memory.emplace_back(current);
            }
            return ptr;
        }
    }
}

char* ASTAllocator::allocate_size(std::size_t obj_size, std::size_t alignment) {
    std::lock_guard<std::mutex> lock(*allocator_mutex);
    const auto ptr = object_heap_pointer(obj_size, alignment);
    store_ptr((ASTAny*) (void*) ptr);
    return ptr;
}

char* ASTAllocator::allocate_with_cleanup(std::size_t obj_size, std::size_t alignment, void* cleanup_fn) {
    std::lock_guard<std::mutex> lock(*allocator_mutex);
    const auto ptr = object_heap_pointer(obj_size, alignment);
    store_cleanup_fn((void*) ptr, cleanup_fn);
    return ptr;
}

char* BatchAllocator::allocate_released_size(std::size_t obj_size, std::size_t alignment) {
    std::lock_guard<std::mutex> lock(*allocator_mutex);
    return object_heap_pointer(obj_size, alignment);
}

char* BatchAllocator::allocate_str(const char* data, std::size_t size) {
    auto ptr = allocate_released_size(sizeof(char) * (size + 1), alignof(char));
    memcpy(ptr, data, size);
    *(ptr + size) = '\0';
    return ptr;
}