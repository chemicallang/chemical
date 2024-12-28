// Copyright (c) Qinetik 2024.

#pragma once

#include "integration/common/Diagnostic.h"
#include "FlatIGFile.h"
#include "cst/base/CSTUnit.h"
#include <memory>

struct LexResult {

    /**
     * the absolute path to the file
     */
    std::string abs_path;
    /**
     * the unit that owns the tokens
     */
    CSTUnit unit;
    /**
     * diagnostics when CSTTokens were lexed / resolved
     */
    std::vector<Diag> diags;

};
