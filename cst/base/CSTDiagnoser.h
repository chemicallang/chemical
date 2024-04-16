// Copyright (c) Qinetik 2024.

#pragma once

#include "common/Diagnostic.h"
#include "CSTToken.h"
#include <string>
#include <vector>

class CSTDiagnoser {
public:

    bool has_errors = false;

    /**
     * diagnostics, containing errors and warning
     */
    std::vector<Diag> diagnostics;

    /**
     * record an error
     */
    void error(const std::string &message, CSTToken *start, CSTToken *end, DiagSeverity severity = DiagSeverity::Error);

    /**
     * record an error
     */
    void error(const std::string &message, CSTToken *inside, DiagSeverity severity = DiagSeverity::Error);

};