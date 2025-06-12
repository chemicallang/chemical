// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "lsp/types.h"
#include "core/diag/Range.h"
#include "ast/base/ASTNode.h"
#include <string>
#include <span>

class LocationManager;

 std::vector<lsp::InlayHint> inlay_hint_analyze(LocationManager& manager, const std::span<ASTNode*>& nodes, const Range& range);