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

    inline void info(std::string& message, SourceLocation location) {
        location_diagnostic(message, location, DiagSeverity::Information);
    }

    inline void info(const std::string_view& message, SourceLocation location) {
        location_diagnostic(message, location, DiagSeverity::Information);
    }

    inline void warn(std::string& message, SourceLocation location) {
        location_diagnostic(message, location, DiagSeverity::Warning);
    }

    inline void warn(const std::string_view& message, SourceLocation location) {
        location_diagnostic(message, location, DiagSeverity::Warning);
    }

    inline void error(std::string& message, SourceLocation location) {
        location_diagnostic(message, location, DiagSeverity::Error);
    }

    inline void error(const std::string_view& message, SourceLocation location) {
        location_diagnostic(message, location, DiagSeverity::Error);
    }

    template <typename NodeT>
    inline void diagnostic(std::string& err, NodeT* node, DiagSeverity severity)
    requires requires(NodeT n) { n.encoded_location(); }
    {
        location_diagnostic(err, node->encoded_location(), severity);
    }

    template <typename NodeT>
    inline void diagnostic(std::string_view err, NodeT* node, DiagSeverity severity)
    requires requires(NodeT n) { n.encoded_location(); }
    {
        location_diagnostic(err, node->encoded_location(), severity);
    }

    template <typename NodeT>
    inline void info(std::string &err, NodeT* node)
    requires requires(NodeT n) { n.encoded_location(); }
    {
        diagnostic(err, node, DiagSeverity::Information);
    }

    template <typename NodeT>
    inline void info(std::string_view err, NodeT* node)
    requires requires(NodeT n) { n.encoded_location(); }
    {
        diagnostic(err, node, DiagSeverity::Information);
    }

    template <typename NodeT>
    inline void warn(std::string &err, NodeT* node)
    requires requires(NodeT n) { n.encoded_location(); }
    {
        diagnostic(err, node, DiagSeverity::Warning);
    }

    template <typename NodeT>
    inline void warn(std::string_view err, NodeT* node)
    requires requires(NodeT n) { n.encoded_location(); }
    {
        diagnostic(err, node, DiagSeverity::Warning);
    }

    template <typename NodeT>
    inline void error(std::string &err, NodeT* node)
    requires requires(NodeT n) { n.encoded_location(); }
    {
        diagnostic(err, node, DiagSeverity::Error);
    }

    template <typename NodeT>
    inline void error(std::string_view err, NodeT* node)
    requires requires(NodeT n) { n.encoded_location(); }
    {
        diagnostic(err, node, DiagSeverity::Error);
    }

    template <typename NodeT1, typename NodeT2>
    inline void info(std::string &err, NodeT1* node, NodeT2* other)
    requires requires(NodeT1 n) { n.encoded_location(); } &&
             requires(NodeT2 n) { n.encoded_location(); }
    {
        diagnostic(err, node, DiagSeverity::Information);
        diagnostic(err, other, DiagSeverity::Information);
    }

    template <typename NodeT1, typename NodeT2>
    inline void info(std::string_view err, NodeT1* node, NodeT2* other)
    requires requires(NodeT1 n) { n.encoded_location(); } &&
             requires(NodeT2 n) { n.encoded_location(); }
    {
        diagnostic(err, node, DiagSeverity::Information);
        diagnostic(err, other, DiagSeverity::Information);
    }

    template <typename NodeT1, typename NodeT2>
    inline void warn(std::string &err, NodeT1* node, NodeT2* other)
    requires requires(NodeT1 n) { n.encoded_location(); } &&
             requires(NodeT2 n) { n.encoded_location(); }
    {
        diagnostic(err, node, DiagSeverity::Warning);
        diagnostic(err, other, DiagSeverity::Warning);
    }

    template <typename NodeT1, typename NodeT2>
    inline void warn(std::string_view err, NodeT1* node, NodeT2* other)
    requires requires(NodeT1 n) { n.encoded_location(); } &&
             requires(NodeT2 n) { n.encoded_location(); }
    {
        diagnostic(err, node, DiagSeverity::Warning);
        diagnostic(err, other, DiagSeverity::Warning);
    }

    template <typename NodeT1, typename NodeT2>
    inline void error(std::string &err, NodeT1* node, NodeT2* other)
    requires requires(NodeT1 n) { n.encoded_location(); } &&
             requires(NodeT2 n) { n.encoded_location(); }
    {
        diagnostic(err, node, DiagSeverity::Error);
        diagnostic(err, other, DiagSeverity::Error);
    }

    template <typename NodeT1, typename NodeT2>
    inline void error(std::string_view err, NodeT1* node, NodeT2* other)
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