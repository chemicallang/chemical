// Copyright (c) Chemical Language Foundation 2025.

#include "rang.hpp"
#include "Diagnostic.h"
#include <iostream>
#include "Diagnoser.h"
#include "rang.hpp"
#include "core/source/LocationManager.h"

Diag Diagnoser::make_diag(const chem::string_view& message, const chem::string_view& filePath, const Position& start, const Position& end, DiagSeverity severity) {
    return Diag(
            Range {
                    start,
                    end
            },
            severity,
            filePath.str(),
            message.str()
    );
}

void Diagnoser::add_diag(Diag diag) {
    if (diag.severity == DiagSeverity::Error) {
        if(early_errors) {
            std::cerr << rang::fg::red << "[Debug_Error] " << diag.message << " at " << diag.path_url.value() << ":"
                      << diag.range.start.representation() << rang::fg::reset << std::endl;
        }
        has_errors = true;
    }
    diagnostics.emplace_back(diag);
}

void Diagnoser::diagnostic(const chem::string_view &message, DiagSeverity severity) {
    diagnostic(message, "", Position { 0, 0 }, Position { 0, 0 }, severity);
}

void Diagnoser::print_diagnostics(std::vector<Diag>& diagnostics, const chem::string_view& path, const chem::string_view& tag) {
    if(!diagnostics.empty()) {
        for (const auto &err: diagnostics) {
            const auto use_cerr = err.severity.has_value() && err.severity.value() == DiagSeverity::Error;
            err.ansi(use_cerr ? std::cerr : std::cout, path, tag) << '\n';
        }
        std::cout << std::flush;
        std::cerr << std::flush;
    }
}

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

void Diag::format(std::ostream& os, const chem::string_view& path, const chem::string_view& tag) const {
    os << '[' << tag << "] " << message << " at " << path << ':' << range.representation();
}

std::ostream& Diag::ansi(std::ostream& os, const chem::string_view& path, const chem::string_view &tag) const {
    color(os, severity.value());
    format(os, path, tag);
    os << rang::bg::reset << rang::fg::reset;
    return os;
}