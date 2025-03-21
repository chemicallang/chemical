// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "cstddef"

class BatchAllocator;

class ASTAllocator;

extern "C" {

    char* BatchAllocatorallocate_size(BatchAllocator* allocator, size_t obj_size, size_t alignment);

}