// Copyright (c) Qinetik 2024.

#pragma once

#include <string>
#include "integration/common/DiagSeverity.h"

struct ASTDiag {
    std::string message;
    DiagSeverity severity;
};