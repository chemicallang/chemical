// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "std/chem_string_view.h"
// TODO remove string import
#include <string>
#include "core/source/SourceLocation.h"


struct LocatedIdentifier {

    /**
     * the actual identifier
     */
    chem::string_view identifier;

};

constexpr LocatedIdentifier ZERO_LOC_ID(const char* str) {
    std::size_t size;
    if(str) {
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE < 8
        size =  __builtin_strlen(Str);
#else
        size = std::char_traits<char>::length(str);
#endif
    } else {
        size = 0;
    }
    return { chem::string_view(str, size) };
}

LocatedIdentifier ZERO_LOC_ID(BatchAllocator& allocator, std::string& identifier);

//LocatedIdentifier LOC_ID(BatchAllocator& allocator, std::string identifier, SourceLocation location);