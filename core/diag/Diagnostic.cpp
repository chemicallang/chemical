// Copyright (c) Chemical Language Foundation 2025.

#include "rang.hpp"
#include "Diagnostic.h"
#include <iostream>
#include "Diagnoser.h"
#include "rang.hpp"
#include "core/source/LocationManager.h"

Diag Diagnoser::make_diag(const chem::string_view& message, const chem::string_view& filePath, const Position& start, const Position& end, DiagSeverity severity, const chem::string_view& code) {
    Diag diag(
            Range {
                    start,
                    end
            },
            severity,
            filePath.str(),
            message.str()
    );
    diag.with_code(code);
    return diag;
}

void Diagnoser::add_diag(Diag diag) {
    if (diag.severity == DiagSeverity::Error) {
        error_count++;
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
            err.ansi(use_cerr ? std::cerr : std::cout, err.path_url.has_value() ? chem::string_view(err.path_url.value()) : path, tag) << '\n';
        }
        std::cout << std::flush;
        std::cerr << std::flush;
    }
}

void Diagnoser::print_diagnostics(std::vector<Diag>& diagnostics, const chem::string_view& tag) {
    if(!diagnostics.empty()) {
        for (const auto &err: diagnostics) {
            const auto use_cerr = err.severity.has_value() && err.severity.value() == DiagSeverity::Error;
            err.ansi(use_cerr ? std::cerr : std::cout, err.path_url.has_value() ? chem::string_view(err.path_url.value()) : "", tag) << '\n';
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
    os << '[' << tag << "] ";
    if(code.has_value()) {
        os << '[' << code.value() << "] ";
    }
    os << message << " at " << path << ':' << range.representation();
    if(relatedInformation.has_value()) {
        for(const auto& related : relatedInformation.value()) {
            os << "\n  note: " << related.message << " at " << path << ':'
               << (related.location.line + 1) << ':' << (related.location.character + 1);
        }
    }
}

std::ostream& Diag::ansi(std::ostream& os, const chem::string_view& path, const chem::string_view &tag) const {
    os << '[' << tag << "] ";
    if (severity.has_value()) {
        switch (severity.value()) {
        case DiagSeverity::Error:
            os << rang::fg::red << "error";
            if(code.has_value()) os << '[' << code.value() << ']';
            os << ": " << rang::fg::reset;
            break;
        case DiagSeverity::Warning:
            os << rang::fg::yellow << "warn";
            if(code.has_value()) os << '[' << code.value() << ']';
            os << ": " << rang::fg::reset;
            break;
        case DiagSeverity::Information:
            os << rang::fg::gray << "info";
            if(code.has_value()) os << '[' << code.value() << ']';
            os << ": " << rang::fg::reset;
            break;
        case DiagSeverity::Hint:
            os << rang::fg::cyan << "hint";
            if(code.has_value()) os << '[' << code.value() << ']';
            os << ": " << rang::fg::reset;
            break;
        }
    }
    os << message << " at " << path << ':' << (range.start.line + 1) << ':' << (range.start.character + 1);
    if(relatedInformation.has_value()) {
        for(const auto& related : relatedInformation.value()) {
            os << "\n  note: " << related.message << " at " << path << ':'
               << (related.location.line + 1) << ':' << (related.location.character + 1);
        }
    }
    return os;
}