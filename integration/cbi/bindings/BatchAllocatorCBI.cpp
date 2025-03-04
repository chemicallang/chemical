// Copyright (c) Chemical Language Foundation 2025.

#include "BatchAllocatorCBI.h"
#include "ast/base/ASTAllocator.h"

char* BatchAllocatorallocate_size(BatchAllocator* allocator, size_t obj_size, size_t alignment) {
    return allocator->allocate_released_size(obj_size, alignment);
}