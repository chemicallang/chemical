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
     * create a diag with parameters
     */
    static Diag make_diag(const chem::string_view& message, const chem::string_view& file_path, const Position& start, const Position& end, DiagSeverity severity);

    /**
     * add the given diagnostic
     */
    void add_diag(Diag diag);

    /**
     * get diagnostic
     */
    Diag& empty_diagnostic(const chem::string_view& file_path, const Position& start, const Position& end, DiagSeverity severity) {
        if(severity == DiagSeverity::Error) {
            has_errors = true;
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
        has_errors = false;
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