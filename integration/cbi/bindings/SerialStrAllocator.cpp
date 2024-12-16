// Copyright (c) Qinetik 2024.

#include "SerialStrAllocatorCBI.h"
#include "std/alloc/SerialStrAllocator.h"

void SerialStrAllocatordeallocate(SerialStrAllocator* allocator) {
    allocator->deallocate();
}

void SerialStrAllocatorcurrent_view(chem::string_view* view, SerialStrAllocator* allocator) {
    *view = allocator->current_view();
}

void SerialStrAllocatorfinalize_view(chem::string_view* view, SerialStrAllocator* allocator) {
    *view = allocator->finalize_view();
}

void SerialStrAllocatorappend(SerialStrAllocator* allocator, char c) {
    allocator->append(c);
}