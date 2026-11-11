// Copyright (c) Chemical Language Foundation 2025.

#include "ASTAllocator.h"
#include <mutex>
#include <cassert>
#include "cstring"
#ifdef CHEM_DEBUG_AST_POISON
#include <new>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif
#include "libtcc.h"

#ifdef CHEM_DEBUG_AST_POISON
/*
 * AST arena poison mode — a *build-time* diagnostic for AST lifetime bugs, off
 * unless `CHEM_DEBUG_AST_POISON` is defined:
 *
 *     cmake -S . -B cmake-build-debug -DCHEM_DEBUG_AST_POISON=ON
 *     ./scripts/build.sh --tcc
 *
 * `clear()` normally releases every block but the first and lets the CRT reuse
 * them, so a dangling pointer into a released block usually reads
 * plausible-looking garbage: a use-after-free of AST nodes then only crashes when
 * the block happens to be unmapped or recycled, which makes the bug appear
 * intermittently (it produced random `RUNTIME ERROR: invalid memory access`
 * aborts in the lab build script).
 *
 * Here every block is a dedicated `VirtualAlloc` region, and `clear()` decommits
 * it and leaves it reserved with `PAGE_NOACCESS` instead of handing it back, and
 * the arena starts from a fresh block. A stale read then faults deterministically
 * instead of reading garbage; combine with `-DCHEM_DEBUG_FAULT_REPORT=ON` to get a
 * symbolized stack for the offending use. Costs nothing when not defined (see
 * `heap_block_sizes` in BatchAllocator.h).
 */
static char* poison_allocate_block(std::size_t size) {
    // a dedicated region per block, so retiring it can never hand the pages to
    // another allocation
    auto* block = static_cast<char*>(
            VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    if (block == nullptr) {
        throw std::bad_alloc();
    }
    return block;
}

static void poison_retire_block(char* block, std::size_t size) {
    // keep the reservation so no later allocation can reuse the address, and
    // make every access fault
    VirtualFree(block, size, MEM_DECOMMIT);
    DWORD previous = 0;
    VirtualProtect(block, size, PAGE_NOACCESS, &previous);
}

static void poison_release_block(char* block) {
    VirtualFree(block, 0, MEM_RELEASE);
}
#endif

BatchAllocator::BatchAllocator(std::size_t heapBatchSize) : heap_offset(heapBatchSize), heap_batch_size(heapBatchSize), initial_heap_batch_size(heapBatchSize) {
    if(heapBatchSize > 0) {
        // reserving a single heap batch size allocation for usage
        // otherwise first allocation will fail
        reserve_heap_storage();
    }
    allocator_mutex = std::make_shared<std::mutex>();
}


ASTAllocator::ASTAllocator(
    std::size_t heapBatchSize
) : BatchAllocator(heapBatchSize) {
    ptr_storage.reserve(PTR_VEC_SIZE);
    cleanup_fns.reserve(PTR_VEC_SIZE);
}

BatchAllocator::BatchAllocator(
        BatchAllocator&& other
) noexcept : heap_memory(std::move(other.heap_memory)),
#ifdef CHEM_DEBUG_AST_POISON
    heap_block_sizes(std::move(other.heap_block_sizes)),
#endif
    heap_batch_size(other.heap_batch_size), initial_heap_batch_size(other.initial_heap_batch_size), heap_offset(other.heap_offset),
    allocator_mutex(std::move(other.allocator_mutex))
{
    other.heap_offset = 0;
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
#ifdef CHEM_DEBUG_AST_POISON
    heap_block_sizes = std::move(other.heap_block_sizes);
#endif
    heap_batch_size = other.heap_batch_size;
    initial_heap_batch_size = other.initial_heap_batch_size;
    heap_offset = other.heap_offset;
    allocator_mutex = std::move(other.allocator_mutex);

    other.heap_offset = 0;

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
#ifdef CHEM_DEBUG_AST_POISON
    // diagnostic build: retire every block instead of releasing it, so a dangling
    // pointer faults instead of reading recycled memory
    for (size_t i = 0; i < heap_memory.size(); i++) {
        poison_retire_block(heap_memory[i], heap_block_sizes[i]);
    }
    heap_memory.clear();
    heap_block_sizes.clear();
    heap_offset = heap_batch_size;
    heap_batch_size = initial_heap_batch_size;
#else
    if(heap_memory.empty()) {
        // force heap allocation
        heap_offset = heap_batch_size;
    } else {
        // delete all but the first block
        for (size_t i = 1; i < heap_memory.size(); ++i) {
            ::operator delete(heap_memory[i]);
        }
        // shrink vector back to size 1
        heap_memory.erase(heap_memory.begin() + 1, heap_memory.end());
        // reset offset so that next allocation will start fresh in that one block
        heap_offset = 0;
        // restore batch size to match the actual size of block[0],
        // which was allocated with the original batch size
        heap_batch_size = initial_heap_batch_size;
    }
#endif
}

void BatchAllocator::destroy_memory() {
    for (const auto heap_ptr: heap_memory) {
#ifdef CHEM_DEBUG_AST_POISON
        poison_release_block(heap_ptr);
#else
        ::operator delete(heap_ptr);
#endif
    }
#ifdef CHEM_DEBUG_AST_POISON
    heap_memory.clear();
    heap_block_sizes.clear();
#endif
}

BatchAllocator::~BatchAllocator() {
    destroy_memory();
    // mutex is managed by shared_ptr, no need to delete it manually
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
#ifdef CHEM_DEBUG_AST_POISON
    const auto heap_pointer = poison_allocate_block(heap_batch_size);
    heap_block_sizes.emplace_back(heap_batch_size);
#else
    const auto heap_pointer = static_cast<char*>(::operator new(heap_batch_size));
#endif
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
