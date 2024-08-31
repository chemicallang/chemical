// Copyright (c) Qinetik 2024.

#pragma once

#include <string>
#include <vector>
#include "cst/base/CSTDiagnoser.h"

class ASTAny;

/**
 * a class that provides helpful methods and fields that allow to separate stuff from codegen
 * that could be useful to other classes, that process AST nodes for codegen purposes
 */
class ASTDiagnoser : public CSTDiagnoser {
public:

    /**
     * constructor
     */
    ASTDiagnoser() = default;

    void info(const std::string &err, ASTAny* node);

    void warn(const std::string &err, ASTAny *node);

    void error(const std::string &err, ASTAny* node);

    [[deprecated]]
    void info(const std::string &msg) {
        diagnostic(msg, DiagSeverity::Information);
    }

    [[deprecated]]
    void warn(const std::string &msg) {
        diagnostic(msg, DiagSeverity::Warning);
    }

    [[deprecated]]
    void error(const std::string &msg) {
        diagnostic(msg, DiagSeverity::Error);
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