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
     * creates a cbi with name cbi_name
     */
    virtual void create_cbi(const std::string& cbi_name) = 0;

    /**
     * imports collected nodes from a container into a cbi
     * @param cbi_name cbi name
     * @param container container name
     */
    virtual void import_container(const std::string& cbi_name, const std::string& container) = 0;

    /**
     * collects the tokens globally
     * @param name is the name of container
     * @param err_no_found should it error out, if container isn't found
     */
    virtual void collect(const std::string& name, std::vector<CSTToken*>& tokens, bool err_no_found) = 0;

    /**
     * following cbi will be compiled
     */
    virtual bool compile(const std::string& cbi_name) = 0;

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