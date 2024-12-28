// Copyright (c) Qinetik 2024.

#include <iostream>
#include "cst/base/CSTDiagnoser.h"
#include "rang.hpp"
#include "cst/LocationManager.h"

Diag CSTDiagnoser::make_diag(const std::string_view& message, const std::string_view& filePath, const Position& start, const Position& end, DiagSeverity severity) {
    return Diag(
            Range {
                    start,
                    end
            },
            severity,
            std::string(filePath),
            std::string(message)
    );
}

void CSTDiagnoser::add_diag(Diag diag) {
    if (diag.severity == DiagSeverity::Error) {
        if(early_errors) {
            std::cerr << rang::fg::red << "[Debug_Error] " << diag.message << " at " << diag.path_url.value() << ":"
                      << diag.range.start.representation() << rang::fg::reset << std::endl;
        }
        has_errors = true;
    }
    diagnostics.emplace_back(diag);
}

void CSTDiagnoser::diagnostic(const std::string_view &message, DiagSeverity severity) {
    diagnostic(message, "", Position { 0, 0 }, Position { 0, 0 }, severity);
}

void CSTDiagnoser::print_diagnostics(std::vector<Diag>& diagnostics, const std::string_view& path, const std::string& tag) {
    for (const auto &err: diagnostics) {
        err.ansi(std::cerr, path, tag) << std::endl;
    }
}