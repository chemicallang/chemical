// Copyright (c) Qinetik 2024.

#pragma once

class BatchAllocator;

extern "C" {

    char* BatchAllocatorallocate_size(BatchAllocator* allocator, size_t obj_size, size_t alignment);

}