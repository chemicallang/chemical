// Copyright (c) Qinetik 2024.

#pragma once

#include "integration/common/Diagnostic.h"
#include "CSTToken.h"
#include "cst/SourceLocation.h"
#include <string>
#include <vector>

class LocationManager;

class CSTDiagnoser {
public:

    /**
     * this can be set to true, to report early errors
     * what it does is report the error and store it as well
     * if our compiler crashes because of an error that happened previously
     * and we stored it, it will be reported as soon as it happens
     * this allows us to support --early-errors command line option
     */
#ifdef DEBUG
    bool early_errors = true;
#else
    bool early_errors = false;
#endif

    /**
     * this is set to true, if a single diagnostic with error severity is added
     * this helps to know if errors were added in any process performed, allowing us to
     * avoid moving forward
     */
    bool has_errors = false;

    /**
     * diagnostics, containing errors and warning
     */
    std::vector<Diag> diagnostics;

    /**
     * the actual diagnostic function that does everything
     */
    void diagnostic(const std::string_view& message, const std::string_view& file_path, const Position& start, const Position& end, DiagSeverity severity);

    /**
     * give diagnostics related to tokens
     */
    void token_diagnostic(const std::string_view& message, const std::string_view& file_path, CSTToken* start, CSTToken* end, DiagSeverity severity);

    /**
     * a helper function
     */
    inline void diagnostic(const std::string_view& message, const std::string_view& filePath, CSTToken* start, CSTToken* end, DiagSeverity severity) {
        token_diagnostic(message, filePath, start, end, severity);
    }

    /**
     * record a diagnostic
     */
    [[deprecated]]
    void diagnostic(const std::string_view& message, DiagSeverity severity);

    /**
     * record a diagnostic
     */
    inline void diagnostic(const std::string_view& message, const std::string_view& filePath, CSTToken *inside, DiagSeverity severity) {
        diagnostic(message, filePath, inside->start_token(), inside->end_token(), severity);
    }

    /**
     * a helper method
     */
    inline void error(const std::string_view& message, const std::string_view& filePath, CSTToken* start, CSTToken* end) {
        diagnostic(message, filePath, start, end, DiagSeverity::Error);
    }

    /**
     * a helper method
     */
    inline void error(const std::string_view& message, const std::string_view& filePath, CSTToken* inside) {
        diagnostic(message, filePath, inside, DiagSeverity::Error);
    }

    /**
     * a helper method
     */
    inline void info(const std::string_view& message, const std::string_view& filePath, CSTToken* start, CSTToken* end) {
        diagnostic(message, filePath, start, end, DiagSeverity::Information);
    }

    /**
     * a helper method
     */
    inline void info(const std::string_view& message, const std::string_view& filePath, CSTToken* inside) {
        diagnostic(message, filePath, inside, DiagSeverity::Information);
    }

    /**
     * a helper method
     */
    inline void warn(const std::string_view& message, const std::string_view& filePath, CSTToken* start, CSTToken* end) {
        diagnostic(message, filePath, start, end, DiagSeverity::Warning);
    }

    /**
     * a helper method
     */
    inline void warn(const std::string_view& message, const std::string_view& filePath, CSTToken* inside) {
        diagnostic(message, filePath, inside, DiagSeverity::Warning);
    }

    /**
     * a helper method
     */
    inline void hint(const std::string_view& message, const std::string_view& filePath, CSTToken* start, CSTToken* end) {
        diagnostic(message, filePath, start, end, DiagSeverity::Hint);
    }

    /**
     * a helper method
     */
    inline void hint(const std::string_view& message, const std::string_view& filePath, CSTToken* inside) {
        diagnostic(message, filePath, inside, DiagSeverity::Hint);
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
    void print_diagnostics(const std::string_view& path, const std::string& tag);

};