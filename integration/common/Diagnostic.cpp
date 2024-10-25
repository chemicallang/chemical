// Copyright (c) Qinetik 2024.

#include "rang.hpp"
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

void color(std::ostream& os, DiagSeverity severity) {
    switch (severity) {
        case DiagSeverity::Error:
            os << rang::fg::red;
            break;
        case DiagSeverity::Warning:
            os << rang::fg::yellow;
            break;
        case DiagSeverity::Information:
            os << rang::fg::gray;
            break;
        case DiagSeverity::Hint:
            os << rang::fg::cyan;
            break;
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

void Diag::format(std::ostream& os, const std::string_view& path, const std::string_view& tag) const {
    os << '[' << tag << "] " << message << " at " << path << ':' << range.representation();
}

std::ostream& Diag::ansi(std::ostream& os, const std::string_view& path, const std::string_view &tag) const {
    color(os, severity.value());
    format(os, path, tag);
    os << rang::bg::reset << rang::fg::reset;
    return os;
}