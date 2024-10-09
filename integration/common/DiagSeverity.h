// Copyright (c) Qinetik 2024.

#pragma once

#include <string>

enum class DiagSeverity : int {
    // Reports an error.
    Error = 1,
    // Reports a warning.
    Warning = 2,
    // Reports an information.
    Information = 3,
    // Reports a hint.
    Hint = 4
};

std::ostream& color(std::ostream& os, DiagSeverity severity);

std::string to_string(DiagSeverity severity);