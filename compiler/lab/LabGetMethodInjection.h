// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "std/chem_string_view.h"

class FunctionDeclaration;
class ASTAllocator;

FunctionDeclaration* default_build_lab_get_method(
        ASTAllocator& allocator,
        const chem::string_view& scopeName,
        const chem::string_view& modName
);