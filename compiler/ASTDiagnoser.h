// Copyright (c) Chemical Language Foundation 2025.

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
    void location_diagnostic(const chem::string_view& message, SourceLocation location, DiagSeverity severity);

    /**
     * record a diagnostic for the given source location
     */
    inline void diagnostic(chem::string_view& message, SourceLocation location, DiagSeverity severity) {
        location_diagnostic(message, location, severity);
    }

    /**
     * get an empty diagnostic to which you can append to
     * @return
     */
    Diag& empty_diagnostic(SourceLocation location, DiagSeverity severity);

    /**
     * create empty diagnostic, to which you can append to
     */
    [[nodiscard]]
    inline Diag& info(SourceLocation location) {
        return empty_diagnostic(location, DiagSeverity::Information);
    }

    /**
     * create empty diagnostic, to which you can append to
     */
    [[nodiscard]]
    inline Diag& warning(SourceLocation location) {
        return empty_diagnostic(location, DiagSeverity::Warning);
    }

    /**
     * create empty diagnostic, to which you can append to
     */
    [[nodiscard]]
    inline Diag& warn(SourceLocation location) {
        return empty_diagnostic(location, DiagSeverity::Warning);
    }

    /**
     * create empty diagnostic, to which you can append to
     */
    [[nodiscard]]
    inline Diag& error(SourceLocation location) {
        return empty_diagnostic(location, DiagSeverity::Error);
    }

    /**
     * create empty diagnostic, to which you can append to
     */
    [[nodiscard]]
    inline Diag& hint(SourceLocation location) {
        return empty_diagnostic(location, DiagSeverity::Hint);
    }

    /**
     * create a info diagnostic
     */
    template <typename NodeT>
    inline Diag& info(NodeT* node)
    requires requires(NodeT n) { n.encoded_location(); }
    {
        return info(node->encoded_location());
    }

    /**
     * create a warning diagnostic
     */
    template <typename NodeT>
    inline Diag& warn(NodeT* node)
    requires requires(NodeT n) { n.encoded_location(); }
    {
        return warn(node->encoded_location());
    }

    /**
     * create a error diagnostic
     */
    template <typename NodeT>
    inline Diag& error(NodeT* node)
    requires requires(NodeT n) { n.encoded_location(); }
    {
        return error(node->encoded_location());
    }

    /**
     * prints the debug location right away
     */
    void print_debug_location(SourceLocation location);

    //-----------
    // OTHER METHODS START HERE THAT MAYBE USING APPEND
    //----------

    inline void info(const chem::string_view& message, SourceLocation location) {
        location_diagnostic(message, location, DiagSeverity::Information);
    }

    inline void warn(const chem::string_view& message, SourceLocation location) {
        location_diagnostic(message, location, DiagSeverity::Warning);
    }

    inline void error(const chem::string_view& message, SourceLocation location) {
        location_diagnostic(message, location, DiagSeverity::Error);
    }

    template <typename NodeT>
    inline void diagnostic(chem::string_view err, NodeT* node, DiagSeverity severity)
    requires requires(NodeT n) { n.encoded_location(); }
    {
        location_diagnostic(err, node->encoded_location(), severity);
    }

    template <typename NodeT>
    inline void info(chem::string_view err, NodeT* node)
    requires requires(NodeT n) { n.encoded_location(); }
    {
        diagnostic(err, node, DiagSeverity::Information);
    }

    template <typename NodeT>
    inline void warn(chem::string_view err, NodeT* node)
    requires requires(NodeT n) { n.encoded_location(); }
    {
        diagnostic(err, node, DiagSeverity::Warning);
    }

    template <typename NodeT>
    inline void error(chem::string_view err, NodeT* node)
    requires requires(NodeT n) { n.encoded_location(); }
    {
        diagnostic(err, node, DiagSeverity::Error);
    }

    template <typename NodeT1, typename NodeT2>
    inline void info(chem::string_view err, NodeT1* node, NodeT2* other)
    requires requires(NodeT1 n) { n.encoded_location(); } &&
             requires(NodeT2 n) { n.encoded_location(); }
    {
        diagnostic(err, node, DiagSeverity::Information);
        diagnostic(err, other, DiagSeverity::Information);
    }

    template <typename NodeT1, typename NodeT2>
    inline void warn(chem::string_view err, NodeT1* node, NodeT2* other)
    requires requires(NodeT1 n) { n.encoded_location(); } &&
             requires(NodeT2 n) { n.encoded_location(); }
    {
        diagnostic(err, node, DiagSeverity::Warning);
        diagnostic(err, other, DiagSeverity::Warning);
    }

    template <typename NodeT1, typename NodeT2>
    inline void error(chem::string_view err, NodeT1* node, NodeT2* other)
    requires requires(NodeT1 n) { n.encoded_location(); } &&
             requires(NodeT2 n) { n.encoded_location(); }
    {
        diagnostic(err, node, DiagSeverity::Error);
        diagnostic(err, other, DiagSeverity::Error);
    }

    /**
     * resets errors
     */
    inline void reset_errors() {
        reset_diagnostics();
    }

};