// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CSTToken.h"
#include "integration/common/Diagnostic.h"
#include "FlatIGFile.h"
#include <memory>

struct LexResult {

    /**
     * the absolute path to the file
     */
    std::string abs_path;
    /**
     * the actual tokens
     */
    std::vector<std::unique_ptr<CSTToken>> tokens;
    /**
     * diagnostics when CSTTokens were lexed / resolved
     */
    std::vector<Diag> diags;

};
