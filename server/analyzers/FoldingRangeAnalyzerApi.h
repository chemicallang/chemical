// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "LibLsp/lsp/textDocument/foldingRange.h"
#include <vector>

class ASTNode;

class LocationManager;

std::vector<FoldingRange> folding_analyze(LocationManager& locMan, std::vector<ASTNode*>& nodes);