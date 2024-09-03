// Copyright (c) Qinetik 2024.

#include <iostream>
#include "cst/base/CSTDiagnoser.h"
#include "cst/base/CSTToken.h"

void CSTDiagnoser::diagnostic(const std::string &message, CSTToken *start, CSTToken *end, DiagSeverity severity) {
    if (severity == DiagSeverity::Error) {
#ifdef DEBUG
        std::cerr << "[Debug_Error] " << message << std::endl;
#endif
        has_errors = true;
    }
    diagnostics.emplace_back(
            Range{
                    start->start_token()->position,
                    end->end_token()->position
            },
            severity,
            std::nullopt,
            message
    );
}

void CSTDiagnoser::diagnostic(const std::string &message, DiagSeverity severity) {
    CSTToken dummy(LexTokenType::Bool, Position(0, 0), "");
    diagnostic(message, &dummy, severity);
}

void CSTDiagnoser::print_diagnostics(const std::string& path, const std::string& tag) {
    for (const auto &err: diagnostics) {
        err.ansi(std::cerr, path, tag) << std::endl;
    }
}