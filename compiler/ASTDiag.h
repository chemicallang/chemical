// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <string>
#include "integration/common/DiagSeverity.h"

struct ASTDiag {
    std::string message;
    DiagSeverity severity;
};