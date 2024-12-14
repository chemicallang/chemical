// Copyright (c) Qinetik 2024.

#include "ASTAllocator.h"
#include <mutex>

BatchAllocator::BatchAllocator(
        char* stackMemory,
        std::size_t stackSize,
        std::size_t heapBatchSize
) : stack_memory(stackMemory), stack_memory_size(stackSize), stack_offset(0),
    heap_offset(heapBatchSize), heap_batch_size(heapBatchSize)
{
    if((stack_memory == nullptr || stackSize == 0) && heapBatchSize > 0) {
        // reserving a single heap batch size allocation for usage
        // otherwise first allocation will fail
        reserve_heap_storage();
    }
    allocator_mutex = new std::mutex;
}


ASTAllocator::ASTAllocator(
    char* stackMemory,
    std::size_t stackSize,
    std::size_t heapBatchSize
) : BatchAllocator(stackMemory, stackSize, heapBatchSize) {
    ptr_storage.reserve(10);
    reserve_ptr_storage();
}

BatchAllocator::BatchAllocator(
        BatchAllocator&& other
) noexcept : stack_memory(other.stack_memory), stack_memory_size(other.stack_memory_size), stack_offset(other.stack_offset),
             heap_memory(std::move(other.heap_memory)), heap_batch_size(other.heap_batch_size), heap_offset(other.heap_offset), allocator_mutex(other.allocator_mutex)
{
    other.stack_memory = nullptr;
    other.stack_memory_size = 0;
    other.stack_offset = 0;
    other.heap_offset = 0;
    other.allocator_mutex = new std::mutex;
}

ASTAllocator::ASTAllocator(
    ASTAllocator&& other
) noexcept : BatchAllocator(std::move(other)), ptr_storage(std::move(other.ptr_storage))
{
}

BatchAllocator& BatchAllocator::operator =(BatchAllocator&& other) noexcept {

    this->~BatchAllocator();

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

ASTAllocator& ASTAllocator::operator =(ASTAllocator&& other) noexcept {
    BatchAllocator::operator=(std::move(other));
    ptr_storage = std::move(other.ptr_storage);
    return *this;
}

std::size_t ASTAllocator::next_allocation_index() {
    const auto storage_size = ptr_storage.size();
#ifdef DEBUG
    if(storage_size < 1) {
        throw std::runtime_error("expected storage size to be always bigger than zero");
    }
#endif
    const auto container_index = storage_size - 1;
    auto& container = ptr_storage[container_index];
    const auto next_index = container.size();
    return (container_index * PTR_VEC_SIZE) + next_index;
}

void ASTAllocator::clear_values_from(std::size_t start) {

    // Calculate the starting vector and offset within that vector
    std::size_t container_index = start / PTR_VEC_SIZE;
    std::size_t offset = start % PTR_VEC_SIZE;

    const auto storage_size = ptr_storage.size();

#ifdef DEBUG
    if(storage_size < 1) {
        throw std::runtime_error("storage must not be empty");
    }
#endif

    // Check if the starting index is valid
    if (container_index >= storage_size || (container_index == storage_size - 1 && offset >= ptr_storage.back().size())) {
        return;
    }

    // Clear the pointers from the start index
    for (std::size_t i = storage_size - 1; i >= container_index;) {
        auto& container = ptr_storage[i];
        if (i == container_index) {
            // Pop back elements from the offset to the end
            if(!container.empty()) {
                auto current = container.size() - 1;
                while(current >= offset) {
                    const auto ptr = container.back();
                    ptr->~ASTAny();
                    container.pop_back();
                    if(current == 0) {
                        break;
                    }
                    current--;
                }
            }
        } else {
            // clear all from the container
            while(!container.empty()) {
                const auto ptr = container.back();
                ptr->~ASTAny();
                container.pop_back();
            }
        }
        if(i == 0) {
            // prevent underflow, when i is already zero
            break;
        }
        i--;
    }

}

void ASTAllocator::clear() {
    std::lock_guard<std::mutex> lock(*allocator_mutex);
    destruct_ptr_storage();
    destroy_memory();
    clear_ptr_storage();
    stack_offset = 0;
    heap_memory.clear();
    heap_offset = heap_batch_size; // force heap allocation
}

void BatchAllocator::destroy_memory() {
    for (auto& heap_ptr: heap_memory) {
        ::operator delete(heap_ptr);
    }
}

BatchAllocator::~BatchAllocator() {
    destroy_memory();
    delete allocator_mutex;
}

void ASTAllocator::destruct_ptr_storage() {
    for (auto& vec: ptr_storage) {
        for (auto& ptr: vec) {
            ptr->~ASTAny();
        }
    }
}

ASTAllocator::~ASTAllocator() {
    destruct_ptr_storage();
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

char* BatchAllocator::reserve_heap_storage() {
    // reserving a heap pointer with stack size
    const auto heap_pointer = static_cast<char*>(::operator new(heap_batch_size));
    heap_memory.emplace_back(heap_pointer);
    // resetting heap storage
    heap_offset = 0;
    // return
    return heap_pointer;
}

char* BatchAllocator::offset_heap(char* const heap_ptr, std::size_t obj_size, std::size_t alignment) {
    std::size_t aligned_offset = (heap_offset + alignment - 1) & ~(alignment - 1);
    const auto ptr = heap_ptr + aligned_offset;
    heap_offset = aligned_offset + obj_size;
    return ptr;
}

char* BatchAllocator::object_heap_pointer(std::size_t obj_size, std::size_t alignment) {
    // std::size_t aligned_heap_offset = get_aligned_heap_offset(alignment);
    // TODO use aligned_heap_offset here
    if((heap_offset + obj_size) < heap_batch_size) {
        return offset_heap(heap_memory.back(), obj_size, alignment);
    } else {
        if(obj_size < heap_batch_size) {
            return offset_heap(reserve_heap_storage(), obj_size, alignment);
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

char* BatchAllocator::allocate_raw(std::size_t obj_size, std::size_t alignment) {
    std::size_t aligned_stack_offset = (stack_offset + alignment - 1) & ~(alignment - 1);
    if (aligned_stack_offset + obj_size < stack_memory_size) {
        const auto ptr = stack_memory + aligned_stack_offset;
        stack_offset = aligned_stack_offset + obj_size;
        return ptr;
    } else {
        return object_heap_pointer(obj_size, alignment);
    }
}

char* ASTAllocator::allocate_size(std::size_t obj_size, std::size_t alignment) {
    std::lock_guard<std::mutex> lock(*allocator_mutex);
    const auto ptr = allocate_raw(obj_size, alignment);
    store_ptr(ptr);
    return ptr;
}

char* BatchAllocator::allocate_released_size(std::size_t obj_size, std::size_t alignment) {
    std::lock_guard<std::mutex> lock(*allocator_mutex);
    return allocate_raw(obj_size, alignment);
}

char* BatchAllocator::allocate_str(const char* data, std::size_t size) {
    auto ptr = allocate_released_size(sizeof(char) * (size + 1), alignof(char));
    std::memcpy(ptr, data, size);
    *(ptr + size) = '\0';
    return ptr;
}