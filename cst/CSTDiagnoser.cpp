// Copyright (c) Qinetik 2024.

#include <iostream>
#include "cst/base/CSTDiagnoser.h"
#include "cst/base/CSTToken.h"
#include "rang.hpp"

void CSTDiagnoser::real_diagnostic(const std::string_view& message, CSTToken *start, CSTToken *end, DiagSeverity severity) {
    if (severity == DiagSeverity::Error) {
#ifdef DEBUG
        std::cerr << rang::fg::red << "[Debug_Error] " << message << rang::fg::reset << std::endl;
#endif
        has_errors = true;
    }
    diagnostics.emplace_back(
            Range{
                    start->start_token()->position(),
                    end->end_token()->position()
            },
            severity,
            std::nullopt,
            std::string(message)
    );
}

void CSTDiagnoser::diagnostic(std::string &message, DiagSeverity severity) {
    CSTToken dummy(LexTokenType::Bool, Position(0, 0), "");
    diagnostic(message, &dummy, severity);
}

void CSTDiagnoser::diagnostic(std::string_view &message, DiagSeverity severity) {
    CSTToken dummy(LexTokenType::Bool, Position(0, 0), "");
    diagnostic(message, &dummy, severity);
}

void CSTDiagnoser::print_diagnostics(const std::string& path, const std::string& tag) {
    for (const auto &err: diagnostics) {
        err.ansi(std::cerr, path, tag) << std::endl;
    }
}