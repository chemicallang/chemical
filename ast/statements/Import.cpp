// Copyright (c) Chemical Language Foundation 2025.

#include "Import.h"
#include <filesystem>
#include "compiler/SymbolResolver.h"
#include "preprocess/ImportPathHandler.h"
#include "ast/base/GlobalInterpretScope.h"

namespace fs = std::filesystem;

void ImportStatement::declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) {

}