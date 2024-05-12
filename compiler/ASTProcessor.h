// Copyright (c) Qinetik 2024.

#pragma once

#include <string>
#include <vector>

class ASTNode;

/**
 * a class that provides helpful methods and fields that allow to separate stuff from codegen
 * that could be useful to other classes, that process AST nodes for codegen purposes
 */
class ASTProcessor {
public:

    /**
     * the path to resources directory
     */
    std::string resources_dir;

    /**
     * path to the current executable, arg[0]
     * this is useful if in the middle of code generation
     * we want to invoke the compiler to get more information !
     */
    std::string curr_exe_path;

    /**
     * root path to the file, the path to file where code gen started
     */
    std::string path;

    /**
     * path to the current file being code_gen
     */
    std::string current_path;

    /**
     * these are the resolved places where system headers paths exist
     * when its empty, its loaded directly by invoking clang (from self)
     * then once we found them we cache them here, for faster invocation next time
     */
    std::vector<std::string> system_headers_paths = {};

    /**
     * errors are stored here
     */
    std::vector<std::string> errors = std::vector<std::string>();

    /**
     * constructor
     */
    ASTProcessor(std::string curr_exe_path, const std::string& path);

    /**
     * get containing system headers directory for the following header
     */
    std::string headers_dir(const std::string &header);

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

};