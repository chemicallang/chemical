// Copyright (c) Qinetik 2024.

#pragma once

#include "std/chem_string_view.h"
// TODO remove string import
#include <string>
#include "cst/SourceLocation.h"


struct LocatedIdentifier {

    /**
     * the actual identifier
     */
    chem::string_view identifier;

    /**
     * location of the identifier
     */
#ifdef LSP_BUILD
    SourceLocation location;
#endif

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
#ifdef LSP_BUILD
    return { chem::string_view(str, size), ZERO_LOC };
#else
    return { chem::string_view(str, size) };
#endif
}

LocatedIdentifier ZERO_LOC_ID(BatchAllocator& allocator, std::string& identifier);

//LocatedIdentifier LOC_ID(BatchAllocator& allocator, std::string identifier, SourceLocation location);