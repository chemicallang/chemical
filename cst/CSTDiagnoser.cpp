// Copyright (c) Qinetik 2024.

#include <iostream>
#include "cst/base/CSTDiagnoser.h"
#include "cst/base/CSTToken.h"

void CSTDiagnoser::error(const std::string &message, CSTToken *start, CSTToken *end, DiagSeverity severity) {
#ifdef DEBUG
    std::cerr << "Debug reporting error " + message << std::endl;
#endif
    if (severity == DiagSeverity::Error) {
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

void CSTDiagnoser::error(const std::string &message, CSTToken *inside, DiagSeverity severity) {
    error(message, inside->start_token(), inside->end_token(), severity);
}