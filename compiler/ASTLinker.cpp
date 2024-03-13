// Copyright (c) Qinetik 2024.


#include "ast/base/ASTNode.h"
#include "ASTLinker.h"

ASTLinker::ASTLinker(std::vector<std::unique_ptr<ASTNode>> nodes) : nodes(std::move(nodes)) {

}