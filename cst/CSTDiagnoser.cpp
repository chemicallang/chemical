// Copyright (c) Qinetik 2024.

#include "cst/base/CSTDiagnoser.h"
#include "lexer/model/tokens/LexToken.h"

void CSTDiagnoser::error(const std::string &message, CSTToken *start, CSTToken *end, DiagSeverity severity) {
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