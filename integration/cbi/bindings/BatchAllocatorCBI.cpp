// Copyright (c) Qinetik 2024.

#include "BatchAllocatorCBI.h"
#include "ast/base/ASTAllocator.h"

char* BatchAllocatorallocate_size(BatchAllocator* allocator, size_t obj_size, size_t alignment) {
    return allocator->allocate_released_size(obj_size, alignment);
}

char* BatchAllocatorallocate_str(BatchAllocator* allocator, char* data, size_t obj_size) {
    return allocator->allocate_str(data, obj_size);
}