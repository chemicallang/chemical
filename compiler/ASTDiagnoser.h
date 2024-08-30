// Copyright (c) Qinetik 2024.

#pragma once

#include <string>
#include <vector>
#include "ASTDiag.h"

class ASTAny;
class ASTNode;
class CSTToken;

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
     * errors are stored here
     */
    std::vector<ASTDiag> errors;

    /**
     * constructor
     */
    ASTDiagnoser() = default;

    /**
     * the tag of the process, which will be appended to errors
     */
    virtual std::string TAG() {
        return "ASTProcessor";
    }


    /**
     * report an info, which is useful for user to know
     */
    void info(const std::string &err, CSTToken *node);

    [[deprecated]]
    void info(const std::string &err) {
        info(err, (CSTToken*) nullptr);
    }

    [[deprecated]]
    void info(const std::string &err, ASTNode* node) {
        info(err, (CSTToken*) nullptr);
    }

    /**
     * report an error when generating a node
     * @param err
     * @param node the node in which error occurred
     */
    void error(const std::string &err, CSTToken *node);

    [[deprecated]]
    void error(const std::string &err) {
        error(err, (CSTToken*) nullptr);
    }

    [[deprecated]]
    void error(const std::string &err, ASTNode* node) {
        error(err, (CSTToken*) nullptr);
    }

    /**
     * report a warning
     */
    void warn(const std::string &err, CSTToken *node);

    [[deprecated]]
    void warn(const std::string &err) {
        warn(err, (CSTToken*) nullptr);
    }

    [[deprecated]]
    void warn(const std::string &err, ASTNode *node) {
        warn(err, (CSTToken*) nullptr);
    }

    /**
     * an early error reports itself early, this is useful if known
     * the program can crash due to bad pointer being returned due to error
     */
    void early_error(const std::string &err, CSTToken *node);

    [[deprecated]]
    void early_error(const std::string &err) {
        early_error(err, (CSTToken*) nullptr);
    }

    [[deprecated]]
    void early_error(const std::string &err, ASTNode *node) {
        early_error(err, (CSTToken*) nullptr);
    }

    /**
     * just prints the errors to std out
     */
    void print_errors(const std::string& path);

    /**
     * resets errors
     */
    void reset_errors();

};