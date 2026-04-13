// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "core/diag/Diagnostic.h"
#include "core/source/SourceLocation.h"
#include <string>
#include <vector>

class LocationManager;

class Diagnoser {
public:

    /**
     * this is set to true, if a single diagnostic with error severity is added
     * this helps to know if errors were added in any process performed, allowing us to
     * avoid moving forward
     */
    unsigned int error_count = 0;

    /**
     * diagnostics, containing errors and warning
     */
    std::vector<Diag> diagnostics;

    /**
     * create a diag with parameters
     */
    static Diag make_diag(const chem::string_view& message, const chem::string_view& file_path, const Position& start, const Position& end, DiagSeverity severity);

    /**
     * add the given diagnostic
     */
    void add_diag(Diag diag);

    /**
     * check
     */
    inline bool has_errors() {
        return error_count > 0;
    }

    /**
     * get diagnostic
     */
    Diag& empty_diagnostic(const chem::string_view& file_path, const Position& start, const Position& end, DiagSeverity severity) {
        if(severity == DiagSeverity::Error) {
            error_count++;
        }
        diagnostics.emplace_back(Range { start, end }, severity, file_path.str(), "");
        return diagnostics.back();
    }

    /**
     * the actual diagnostic function that does everything
     */
    void diagnostic(const chem::string_view& message, const chem::string_view& file_path, const Position& start, const Position& end, DiagSeverity severity) {
        add_diag(make_diag(message, file_path, start, end, severity));
    }

    /**
     * record a diagnostic
     */
    [[deprecated]]
    void diagnostic(const chem::string_view& message, DiagSeverity severity);

    /**
     * resets the diagnostics
     */
    void reset_diagnostics() {
        error_count = 0;
        diagnostics.clear();
    }

    /**
     * would print the diagnostics to console
     */
    static void print_diagnostics(std::vector<Diag>& diagnostics, const chem::string_view& path, const chem::string_view& tag);

    /**
     * print the diagnostics, uses contained file path
     */
    static void print_diagnostics(std::vector<Diag>& diagnostics, const chem::string_view& tag);

    /**
     * would print diagnostics to console
     */
    void print_diagnostics(const chem::string_view& path, const chem::string_view& tag) {
        print_diagnostics(diagnostics, path, tag);
    }
    /**
     * would print diagnostics to console
     */
    void print_diagnostics(const chem::string_view& tag) {
        print_diagnostics(diagnostics, tag);
    }

};