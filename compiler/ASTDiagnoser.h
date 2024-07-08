// Copyright (c) Qinetik 2024.

#pragma once

#include <string>
#include <vector>
#include "ASTDiag.h"

class ASTNode;

/**
 * a class that provides helpful methods and fields that allow to separate stuff from codegen
 * that could be useful to other classes, that process AST nodes for codegen purposes
 */
class ASTDiagnoser {
public:

    /**
     * set to true, if a diagnostic with error severity is added
     */
    bool has_errors = false;

    /**
     * path to the current file being code_gen
     */
    std::string current_path;

    /**
     * errors are stored here
     */
    std::vector<ASTDiag> errors;

    /**
     * constructor
     */
    ASTDiagnoser(const std::string& path);

    /**
     * the tag of the process, which will be appended to errors
     */
    virtual std::string TAG() {
        return "ASTProcessor";
    }


    /**
     * report an info, which is useful for user to know
     */
    void info(const std::string &err, ASTNode *node = nullptr);

    /**
     * report an error when generating a node
     * @param err
     * @param node the node in which error occurred
     */
    void error(const std::string &err, ASTNode *node = nullptr);

    /**
     * just prints the errors to std out
     */
    void print_errors();

    /**
     * just prints the errors to std out
     */
    void print_errors(const std::string& path);

    /**
     * resets errors
     */
    void reset_errors();

};