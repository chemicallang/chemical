// Copyright (c) Chemical Language Foundation 2025.

#include <iostream>
#include "cst/base/CSTDiagnoser.h"
#include "rang.hpp"
#include "cst/LocationManager.h"

Diag CSTDiagnoser::make_diag(const chem::string_view& message, const chem::string_view& filePath, const Position& start, const Position& end, DiagSeverity severity) {
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

void CSTDiagnoser::diagnostic(const chem::string_view &message, DiagSeverity severity) {
    diagnostic(message, "", Position { 0, 0 }, Position { 0, 0 }, severity);
}

void CSTDiagnoser::print_diagnostics(std::vector<Diag>& diagnostics, const chem::string_view& path, const chem::string_view& tag) {
    if(!diagnostics.empty()) {
        for (const auto &err: diagnostics) {
            err.ansi(std::cerr, path, tag) << '\n';
        }
        std::cerr << std::flush;
    }
}