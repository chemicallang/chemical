// Copyright (c) Qinetik 2024.

#include <vector>
#include "ast/base/ASTAllocator.h"

class ASTNode;

std::vector<ASTNode*> TranslateC(
        ASTAllocator& allocator,
        std::vector<std::string>& args,
        const char *resources_path
);

std::vector<ASTNode*> TranslateC(
        ASTAllocator& allocator,
        const char *exe_path,
        const char *abs_path,
        const char *resources_path
);