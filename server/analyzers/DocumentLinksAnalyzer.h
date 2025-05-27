// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "lsp/types.h"
#include <vector>
#include "integration/common/Location.h"

class LexResult;

class DocumentLinksAnalyzer {
public:

    /**
     * this function analyzes the import unit, in which last file is the one which contains the
     * token where user asked to goto def
     * It will provide locations, where that symbol has definition
     */
    std::vector<lsp::DocumentLink> analyze(LexResult* result, const std::string& compiler_exe_path, const std::string& lsp_exe_path);

};