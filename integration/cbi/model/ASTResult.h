// Copyright (c) Qinetik 2024.

#pragma once

#include <memory>
#include "LexResult.h"
#include "ast/base/ASTUnit.h"

/**
 * ASTResult is converting CSTUnit to a ASTUnit
 * We store diagnostics and other stuff so they can be reported
 */
struct ASTResult {

    /**
     * the absolute path to the file
     */
    std::string abs_path;

    /**
     * the unit that owns the nodes
     */
    ASTUnit unit;

    /**
     * ast allocator here is used to allocate everything inside the ast unit
     */
    ASTAllocator allocator;

    /**
     * diagnostics for this file
     */
    std::vector<Diag> diags;

};