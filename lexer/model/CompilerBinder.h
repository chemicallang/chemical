// Copyright (c) Qinetik 2024.

#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include "integration/ide/bindings/LexerCBI.h"

class Lexer;

class CSTToken;

/**
 * this function is a lex function, it takes the lexer cbi
 * which allows user to lex tokens
 */
typedef void(*cbi_lex_func)(LexerCBI* cbi);

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
     * We keep collected nodes here
     * // TODO shrink these needs before putting in this vector
     */
    std::vector<std::unique_ptr<ASTNode>> collected;

    /**
     * init is used to initialize the compiler binder
     * if it's never called, it's assumed compiler binder isn't required
     */
    virtual void init() = 0;

    /**
     * compile the given tokens
     * @return true if compilation succeeded, false if it did not
     */
    virtual bool compile(std::vector<std::unique_ptr<CSTToken>>& tokens) = 0;

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
     * provides a pointer to function
     */
    virtual void* provide_func(const std::string& funcName) = 0;

    /**
     * provides a function contained within struct
     */
    inline void* provide_func(const std::string& structName, const std::string& funcName) {
        return provide_func(func_name(structName, funcName));
    }

    /**
     * a lex function type is searched in the symbols, if not found nullptr is returned
     * otherwise the pointer to the lex function that can compile the tokens is provided
     * @param structName is the container struct for which it was generated
     */
    inline cbi_lex_func provide_lex_func(const std::string& structName) {
        return (cbi_lex_func) provide_func(structName, "lex");
    }

    /**
     * this function is called to reset for a new file
     * compiler binder share state across files
     * this function only cleans up token references / position for error reporting
     */
    virtual void reset_new_file() = 0;

    /**
     * destructor
     */
    virtual ~CompilerBinder() = default;

};