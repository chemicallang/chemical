// Copyright (c) Qinetik 2024.

#include "Diagnostic.h"

std::string color(DiagSeverity severity) {
    switch (severity) {
        case DiagSeverity::Error:
            return "\x1b[91m";
        case DiagSeverity::Warning:
            return "\x1b[93m";
        case DiagSeverity::Information:
            return "\x1b[94m";
        case DiagSeverity::Hint:
            return "\x1b[96m";
        default:
            return "no color for, unknown diag severity";
    }
}