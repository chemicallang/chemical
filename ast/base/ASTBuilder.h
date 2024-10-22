// Copyright (c) Qinetik 2024.

#pragma once

#include "ASTAllocator.h"

class ASTBuilder {
public:

    ASTAllocator& allocator;

    ASTBuilder(ASTAllocator& allocator) : allocator(allocator) {

    }

};