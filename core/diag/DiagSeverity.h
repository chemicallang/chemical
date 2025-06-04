// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <string>

enum class DiagSeverity : int {
    // Reports an error.
    Error,
    // Reports a warning.
    Warning,
    // Reports an information.
    Information,
    // Reports a hint.
    Hint
};

void color(std::ostream& os, DiagSeverity severity);

std::string to_string(DiagSeverity severity);