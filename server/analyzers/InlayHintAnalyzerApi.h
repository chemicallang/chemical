// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "LibLsp/lsp/textDocument/inlayHint.h"
#include <string>

class LocationManager;

class ASTImportUnitRef;

std::vector<lsInlayHint> inlay_hint_analyze(LocationManager& manager, ASTImportUnitRef& result, const std::string& compiler_exe_path, const std::string& lsp_exe_path);