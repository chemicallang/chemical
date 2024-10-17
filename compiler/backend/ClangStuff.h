// Copyright (c) Qinetik 2024.

#include <vector>
#include "ast/base/ASTAllocator.h"

class ASTNode;

std::vector<ASTNode*> TranslateC(
        ASTAllocator& allocator,
        std::vector<std::string>& args,
        const char *resources_path
);