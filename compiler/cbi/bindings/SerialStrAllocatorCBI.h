// Copyright (c) Chemical Language Foundation 2025.

#pragma once

namespace chem {
    class string_view;
}

class SerialStrAllocator;

extern "C" {

    void SerialStrAllocatordeallocate(SerialStrAllocator* allocator);

    void SerialStrAllocatorcurrent_view(chem::string_view* view, SerialStrAllocator* allocator);

    void SerialStrAllocatorfinalize_view(chem::string_view* view, SerialStrAllocator* allocator);

    void SerialStrAllocatorappend(SerialStrAllocator* allocator, char c);

}