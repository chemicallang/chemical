// Copyright (c) Qinetik 2024.

#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include "CBIData.h"

class CSTDiagnoser;

class LexerCBI;

class ASTProcessor;

/**
 * this function is a lex function, it takes the lexer cbi
 * which allows user to lex tokens
 */
typedef void(*cbi_lex_func)(LexerCBI* cbi);

/**
 * a compile result
 */
struct BinderResult {
    int result;
    std::string error;
};

/**
 * The job of the compiler binder is to bind compiler into user's source code
 * This allows user to call functions inside the compiler to change compiler functioning
 *
 * CompilerBinder basically compiles user source code which contains functions
 * which are called, with a compiler binding interface as a parameter, these functions use the interface
 * to interact with the compiler
 */
class CompilerBinder {
public:

    /**
     * cached pointers of functions
     */
    std::unordered_map<std::string, void*> cached_func;

    /**
     * constructor
     */
    CompilerBinder();

    /**
     * following c translated program is compiled under the given cbi name
     * @param imports absolute paths to files imported in this module
     * @param current_files absolute paths to files in this module
     */
    virtual BinderResult compile(
        const std::string& cbi_name,
        const std::string& program,
        CBIData& cbiData,
        std::vector<std::string_view>& imports,
        std::vector<std::string_view>& current_files,
        ASTProcessor& processor
    ) = 0;

    /**
     * de caches the function given
     */
    inline void de_cache_func(const std::string& funcName) {
        cached_func.erase(funcName);
    }

    /**
     * combines struct name and function name to create full function name
     */
    inline std::string func_name(const std::string& structName, const std::string& funcName) {
        return structName + funcName;
    }

    /**
     * de caches function
     */
    inline void de_cache_func(const std::string& structName, const std::string& funcName) {
        cached_func.erase(func_name(structName, funcName));
    }

    /**
     * provides a function contained within struct
     */
    virtual void* provide_func(const std::string& cbi_name, const std::string& funcName) = 0;

    /**
     * a lex function type is searched in the symbols, if not found nullptr is returned
     * otherwise the pointer to the lex function that can compile the tokens is provided
     * @param structName is the container struct for which it was generated
     */
    inline cbi_lex_func provide_lex_func(const std::string& structName) {
        return (cbi_lex_func) provide_func(structName, func_name(structName, "lex"));
    }

    /**
     * destructor
     */
    virtual ~CompilerBinder() = default;

};