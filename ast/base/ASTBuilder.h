// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ASTAllocator.h"

class ASTBuilder {
public:

    ASTAllocator& allocator;

    ASTBuilder(ASTAllocator& allocator) : allocator(allocator) {

    }

};