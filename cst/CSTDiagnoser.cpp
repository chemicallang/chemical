// Copyright (c) Qinetik 2024.

#include <iostream>
#include "cst/base/CSTDiagnoser.h"
#include "cst/base/CSTToken.h"
#include "rang.hpp"
#include "cst/LocationManager.h"

void CSTDiagnoser::diagnostic(const std::string_view& message, const std::string_view& filePath, const Position& start, const Position& end, DiagSeverity severity) {
    if (severity == DiagSeverity::Error) {
        if(early_errors) {
            std::cerr << rang::fg::red << "[Debug_Error] " << message << " at " << filePath << ":"
                      << start.representation() << rang::fg::reset << std::endl;
        }
        has_errors = true;
    }
    diagnostics.emplace_back(
            Range {
                start,
                end
            },
            severity,
            std::nullopt,
            std::string(message)
    );
}

void CSTDiagnoser::token_diagnostic(const std::string_view& message, const std::string_view& file_path, CSTToken* start, CSTToken* end, DiagSeverity severity) {
    const auto& startPos = start->start_token()->position();
    const auto end_token = end->end_token();
    diagnostic(message, file_path, startPos, { end_token->lineNumber(), end_token->lineCharNumber() + end_token->length() }, severity);
}

void CSTDiagnoser::diagnostic(const std::string_view &message, DiagSeverity severity) {
    CSTToken dummy(LexTokenType::Bool, Position(0, 0), "");
    diagnostic(message, "", &dummy, severity);
}

void CSTDiagnoser::print_diagnostics(const std::string_view& path, const std::string& tag) {
    for (const auto &err: diagnostics) {
        err.ansi(std::cerr, path, tag) << std::endl;
    }
}