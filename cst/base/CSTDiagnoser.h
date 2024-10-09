// Copyright (c) Qinetik 2024.

#pragma once

#include "integration/common/Diagnostic.h"
#include "CSTToken.h"
#include <string>
#include <vector>

class CSTDiagnoser {
private:

    void real_diagnostic(const std::string_view& message, CSTToken *start, CSTToken *end, DiagSeverity severity);

public:

    bool has_errors = false;

    /**
     * diagnostics, containing errors and warning
     */
    std::vector<Diag> diagnostics;

    /**
     * a helper function
     */
    inline void diagnostic(std::string_view& message, CSTToken* start, CSTToken* end, DiagSeverity severity) {
        real_diagnostic(message, start, end, severity);
    }

    /**
     * just a helper function
     */
    inline void diagnostic(std::string& message, CSTToken *start, CSTToken *end, DiagSeverity severity) {
        real_diagnostic(message, start, end, severity);
    }

    /**
     * record a diagnostic
     */
    [[deprecated]]
    void diagnostic(std::string &message, DiagSeverity severity);

    /**
     * record a diagnostic
     */
    [[deprecated]]
    void diagnostic(std::string_view &message, DiagSeverity severity);

    /**
     * record an diagnostic
     */
    inline void diagnostic(std::string& message, CSTToken *inside, DiagSeverity severity) {
        inside ? (
                diagnostic(message, inside->start_token(), inside->end_token(), severity)
        ) : (
                diagnostic(message, severity)
        );
    }

    /**
     * record an diagnostic
     */
    inline void diagnostic(std::string_view& message, CSTToken *inside, DiagSeverity severity) {
        inside ? (
                diagnostic(message, inside->start_token(), inside->end_token(), severity)
        ) : (
                diagnostic(message, severity)
        );
    }

    /**
     * a helper method
     */
    inline void error(std::string& message, CSTToken* start, CSTToken* end) {
        diagnostic(message, start, end, DiagSeverity::Error);
    }

    /**
     * a helper method
     */
    inline void error(std::string& message, CSTToken* inside) {
        diagnostic(message, inside, DiagSeverity::Error);
    }

    /**
     * a helper method
     */
    inline void error(std::string_view message, CSTToken* inside) {
        diagnostic(message, inside, DiagSeverity::Error);
    }

    /**
     * a helper method
     */
    inline void info(std::string& message, CSTToken* start, CSTToken* end) {
        diagnostic(message, start, end, DiagSeverity::Information);
    }

    /**
     * a helper method
     */
    inline void info(std::string& message, CSTToken* inside) {
        diagnostic(message, inside, DiagSeverity::Information);
    }

    /**
     * a helper method
     */
    inline void warn(std::string& message, CSTToken* start, CSTToken* end) {
        diagnostic(message, start, end, DiagSeverity::Warning);
    }

    /**
     * a helper method
     */
    inline void warn(std::string& message, CSTToken* inside) {
        diagnostic(message, inside, DiagSeverity::Warning);
    }

    /**
     * a helper method
     */
    inline void hint(std::string& message, CSTToken* start, CSTToken* end) {
        diagnostic(message, start, end, DiagSeverity::Hint);
    }

    /**
     * a helper method
     */
    inline void hint(std::string& message, CSTToken* inside) {
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