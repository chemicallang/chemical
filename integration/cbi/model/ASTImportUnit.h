// Copyright (c) Qinetik 2024.

#pragma once

#include <memory>
#include <vector>

class ASTResult;

struct ASTImportUnit {

    std::vector<std::shared_ptr<ASTResult>> files;

};