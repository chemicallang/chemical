// Copyright (c) Qinetik 2024.

#pragma once

#include <string>
#include <vector>
#include "cst/base/CSTDiagnoser.h"
#include "ast/base/ASTAny.h"

class Value;

class BaseType;

class ASTNode;

/**
 * a class that provides helpful methods and fields that allow to separate stuff from codegen
 * that could be useful to other classes, that process AST nodes for codegen purposes
 */
class ASTDiagnoser : public CSTDiagnoser {
public:

    /**
     * the location manager is used to decode locations
     */
    LocationManager& loc_man;

    /**
     * the constructor
     */
    ASTDiagnoser(LocationManager& loc_man) : loc_man(loc_man) {

    }

    /**
     * give a diagnostic related to the source location
     */
    void location_diagnostic(const std::string_view& message, SourceLocation location, DiagSeverity severity);

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
     * a diagnostic with severity
     */
    void diagnostic(std::string& err, ASTAny* node, DiagSeverity severity) {
        location_diagnostic(err, node->encoded_location(), severity);
    }

    /**
     * a diagnostic with severity
     */
    void diagnostic(std::string_view& err, ASTAny* node, DiagSeverity severity) {
        location_diagnostic(err, node->encoded_location(), severity);
    }

    /**
     * an info diagnostic
     */
    inline void info(std::string &err, ASTAny* node) {
        diagnostic(err, node, DiagSeverity::Information);
    }

    /**
     * an info diagnostic
     */
    inline void info(std::string_view err, ASTAny* node) {
        diagnostic(err, node, DiagSeverity::Information);
    }

    /**
     * an warning diagnostic
     */
    void warn(std::string &err, ASTAny *node) {
        diagnostic(err, node, DiagSeverity::Warning);
    }

    /**
     * an warning diagnostic
     */
    void warn(std::string_view err, ASTAny *node) {
        diagnostic(err, node, DiagSeverity::Warning);
    }

    /**
     * an error diagnostic
     */
    void error(std::string &err, ASTAny* node) {
        diagnostic(err, node, DiagSeverity::Error);
    }

    /**
     * an error diagnostic
     */
    inline void error(std::string_view err, ASTAny* node) {
        diagnostic(err, node, DiagSeverity::Error);
    }

    /**
     * this will report two diagnostics, one at the location of first node and other
     * at the location of other node
     */
    void info(std::string &err, ASTAny* node, ASTAny* other) {
        diagnostic(err, node, DiagSeverity::Information);
        diagnostic(err, other, DiagSeverity::Information);
    }

    /**
     * this will report two diagnostics, one at the location of first node and other
     * at the location of other node
     */
    void info(std::string_view err, ASTAny* node, ASTAny* other) {
        diagnostic(err, node, DiagSeverity::Information);
        diagnostic(err, other, DiagSeverity::Information);
    }

    /**
     * this will report two diagnostics, one at the location of first node and other
     * at the location of other node
     */
    void warn(std::string &err, ASTAny *node, ASTAny* other) {
        diagnostic(err, node, DiagSeverity::Warning);
        diagnostic(err, other, DiagSeverity::Warning);
    }

    /**
     * this will report two diagnostics, one at the location of first node and other
     * at the location of other node
     */
    void warn(std::string_view err, ASTAny *node, ASTAny* other) {
        diagnostic(err, node, DiagSeverity::Warning);
        diagnostic(err, other, DiagSeverity::Warning);
    }

    /**
     * this will report two diagnostics, one at the location of first node and other
     * at the location of other node
     */
    void error(std::string &err, ASTAny* node, ASTAny* other) {
        diagnostic(err, node, DiagSeverity::Error);
        diagnostic(err, other, DiagSeverity::Error);
    }

    /**
     * this will report two diagnostics, one at the location of first node and other
     * at the location of other node
     */
    void error(std::string_view err, ASTAny* node, ASTAny* other) {
        diagnostic(err, node, DiagSeverity::Error);
        diagnostic(err, other, DiagSeverity::Error);
    }

    /**
     * just prints the errors to std out
     */
    [[deprecated]]
    inline void print_errors(const std::string& path) {
        print_diagnostics(path, "Diagnostic");
    }

    /**
     * resets errors
     */
    inline void reset_errors() {
        reset_diagnostics();
    }

};