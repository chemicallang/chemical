// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "lsp/types.h"
#include <vector>

class ASTNode;

class LocationManager;

std::vector<lsp::FoldingRange> folding_analyze(LocationManager& locMan, std::vector<ASTNode*>& nodes);