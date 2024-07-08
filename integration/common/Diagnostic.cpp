// Copyright (c) Qinetik 2024.

#include "Diagnostic.h"

bool Position::is_ahead(const Position& position) const {
    return line > position.line ||
           (line == position.line && character > position.character);
}

bool Position::is_behind(const Position& position) const {
    return line < position.line ||
           (line == position.line && character < position.character);
}

bool Position::is_equal(const Position& position) const {
    return line == position.line && character == position.character;
}

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
            return "[UnknownDiagSeverity]";
    }
}

std::string to_string(DiagSeverity severity) {
    switch (severity) {
        case DiagSeverity::Error:
            return "ERROR";
        case DiagSeverity::Warning:
            return "WARN";
        case DiagSeverity::Information:
            return "INFO";
        case DiagSeverity::Hint:
            return "HINT";
        default:
            return "[UnknownDiagSeverity]";
    }
}