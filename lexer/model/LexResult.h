// Copyright (c) Qinetik 2024.

#include "lexer/Lexer.h"

struct LexResult {
    std::vector<std::unique_ptr<CSTToken>> tokens;
    std::vector<Diag> diags;
};
