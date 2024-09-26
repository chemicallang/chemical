// Copyright (c) Qinetik 2024.

#pragma once

#include <memory>
#include <vector>
#include "ast/base/GlobalInterpretScope.h"

class ASTResult;

struct ASTImportUnit {

    /**
     * the global allocator responsible for all top level allocations in these files
     */
    ASTAllocator global_allocator;

    /**
     * the local allocator responsible for all local allocations in these files
     */
    ASTAllocator local_allocator;

    /**
     * the global interpret scope is used for all these files
     */
    GlobalInterpretScope comptime_scope;

    /**
     * all the files ast result
     */
    std::vector<std::shared_ptr<ASTResult>> files;

};