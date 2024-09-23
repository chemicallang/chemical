// Copyright (c) Qinetik 2024.

#pragma once

#include <vector>

enum class CBIImportKind : int {
    Lexer = 0,
};

struct CBIImport {
    CBIImportKind kind;
};

struct CBIData {
    std::vector<CBIImport> cbiTypes;
};