// Copyright (c) Qinetik 2024.

#pragma once

#include <memory>
#include <vector>
#include "ast/base/GlobalInterpretScope.h"

class ASTResult;

struct ASTImportUnit {

    /**
     * The allocator used for global interpret scope
     */
    ASTAllocator allocator;

    /**
     * the global interpret scope is used for all these files
     */
    GlobalInterpretScope comptime_scope;

    /**
     * all the files ast result
     */
    std::vector<std::shared_ptr<ASTResult>> files;

};