// Copyright (c) Chemical Language Foundation 2025.

#include "ASTAllocator.h"
#include <mutex>
#include <cassert>
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
// Ensure alignment is a power of two.
static inline bool is_power_of_two(std::size_t x) {
    return (x & (x - 1)) == 0;
}
char* BatchAllocator::object_heap_pointer(std::size_t obj_size, std::size_t alignment) {
    assert(is_power_of_two(alignment) && "Alignment must be a power of two");
    // Calculate the next aligned offset in the current block.
    std::size_t aligned_heap_offset = (heap_offset + alignment - 1) & ~(alignment - 1);
    // Check if the object fits exactly or within the remaining space.
    if ((aligned_heap_offset + obj_size) <= heap_batch_size) {  // <= allows an exact fit.
        char* ret = heap_memory.back() + aligned_heap_offset;
        heap_offset = aligned_heap_offset + obj_size;
        assert(((uintptr_t)ret % alignment) == 0 && "Returned pointer is not properly aligned");
        return ret;
    } else {
        // If the object fits in a new block, reserve new heap storage.
        if (obj_size > heap_batch_size) {
            // update the heap batch size to new size
            heap_batch_size = obj_size;
        }
        char* newBlock = reserve_heap_storage();
        aligned_heap_offset = (0 + alignment - 1) & ~(alignment - 1);
        char* ret = newBlock + aligned_heap_offset;
        heap_offset = aligned_heap_offset + obj_size;
        assert(((uintptr_t)ret % alignment) == 0 && "Returned pointer is not properly aligned");
        return ret;
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