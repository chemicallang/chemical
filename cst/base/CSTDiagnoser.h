// Copyright (c) Qinetik 2024.

#pragma once

#include "integration/common/Diagnostic.h"
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
     * record an diagnostic
     */
    void diagnostic(const std::string &message, CSTToken *start, CSTToken *end, DiagSeverity severity);

    /**
     * record a diagnostic
     */
    [[deprecated]]
    void diagnostic(const std::string &message, DiagSeverity severity);

    /**
     * record an diagnostic
     */
    inline void diagnostic(const std::string &message, CSTToken *inside, DiagSeverity severity) {
        inside ? (
                diagnostic(message, inside->start_token(), inside->end_token(), severity)
        ) : (
                diagnostic(message, severity)
        );
    }

    /**
     * a helper method
     */
    inline void error(const std::string& message, CSTToken* start, CSTToken* end) {
        diagnostic(message, start, end, DiagSeverity::Error);
    }

    /**
     * a helper method
     */
    inline void error(const std::string& message, CSTToken* inside) {
        diagnostic(message, inside, DiagSeverity::Error);
    }

    /**
     * a helper method
     */
    inline void info(const std::string& message, CSTToken* start, CSTToken* end) {
        diagnostic(message, start, end, DiagSeverity::Information);
    }

    /**
     * a helper method
     */
    inline void info(const std::string& message, CSTToken* inside) {
        diagnostic(message, inside, DiagSeverity::Information);
    }

    /**
     * a helper method
     */
    inline void warn(const std::string& message, CSTToken* start, CSTToken* end) {
        diagnostic(message, start, end, DiagSeverity::Warning);
    }

    /**
     * a helper method
     */
    inline void warn(const std::string& message, CSTToken* inside) {
        diagnostic(message, inside, DiagSeverity::Warning);
    }

    /**
     * a helper method
     */
    inline void hint(const std::string& message, CSTToken* start, CSTToken* end) {
        diagnostic(message, start, end, DiagSeverity::Hint);
    }

    /**
     * a helper method
     */
    inline void hint(const std::string& message, CSTToken* inside) {
        diagnostic(message, inside, DiagSeverity::Hint);
    }

    /**
     * resets the diagnostics
     */
    void reset_diagnostics() {
        has_errors = false;
        diagnostics.clear();
    }

    /**
     * would print diagnostics to console
     */
    void print_diagnostics(const std::string& path, const std::string& tag);

};