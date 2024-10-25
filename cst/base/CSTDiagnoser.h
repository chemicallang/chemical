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
     * the reference to location manager is required to
     * decode the locations fo diagnostics
     */
    LocationManager& loc_man;

    /**
     * a very simple constructor
     */
    CSTDiagnoser(LocationManager& loc_man) : loc_man(loc_man) {

    }

    /**
     * the actual diagnostic function that does everything
     */
    void diagnostic(const std::string_view& message, unsigned int file_id, const Position& start, const Position& end, DiagSeverity severity);

    /**
     * give diagnostics related to tokens
     */
    void token_diagnostic(const std::string_view& message, unsigned int file_id, CSTToken* start, CSTToken* end, DiagSeverity severity);

    /**
     * give a diagnostic related to the source location
     */
    void location_diagnostic(const std::string_view& message, SourceLocation location, DiagSeverity severity);

    /**
     * a helper function
     */
    inline void diagnostic(std::string_view& message, CSTToken* start, CSTToken* end, DiagSeverity severity) {
        // TODO the file id is required to report early errors
        token_diagnostic(message, 0, start, end, severity);
    }

    /**
     * just a helper function
     */
    inline void diagnostic(std::string& message, CSTToken *start, CSTToken *end, DiagSeverity severity) {
        // TODO the file id is required to report early errors
        token_diagnostic(message, 0, start, end, severity);
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
     * record a diagnostic for the given source location
     */
    inline void diagnostic(std::string& message, SourceLocation location, DiagSeverity severity) {
        location_diagnostic(message, location, severity);
    }

    /**
     * record a diagnostic for the given source location
     */
    inline void diagnostic(std::string_view& message, SourceLocation location, DiagSeverity severity) {
        location_diagnostic(message, location, severity);
    }

    /**
     * record a diagnostic
     */
    inline void diagnostic(std::string& message, CSTToken *inside, DiagSeverity severity) {
        inside ? (
                diagnostic(message, inside->start_token(), inside->end_token(), severity)
        ) : (
                diagnostic(message, severity)
        );
    }

    /**
     * record a diagnostic
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
    void print_diagnostics(const std::string_view& path, const std::string& tag);

};