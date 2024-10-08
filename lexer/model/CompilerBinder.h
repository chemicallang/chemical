// Copyright (c) Qinetik 2024.

#pragma once

#include "CBIData.h"
#include <unordered_map>

class ASTProcessor;

class Lexer;

class CSTConverter;

class CSTToken;

/**
 * a compile result
 */
struct BinderResult {

    TCCState* module;
    std::string error;

    /**
     * constructor
     */
    BinderResult(std::string err) : error(std::move(err)), module(nullptr) {

    }

    /**
     * constructor
     */
    BinderResult(TCCState* module) : module(module) {

    }

};

/**
 * this function is a lex function, it takes the lexer cbi
 * which allows user to lex tokens
 */
typedef void(*cbi_lex_macro_func)(Lexer* cbi);

/**
 * this function is a lex function, it takes the lexer cbi
 * which allows user to lex tokens
 */
typedef void(*cbi_parse_macro_func)(CSTConverter* cbi, CSTToken* token);

/**
 * compiler binder based on tiny c compiler
 */
class CompilerBinder {
public:

    /**
     * cached pointers of functions
     */
    std::unordered_map<std::string, void*> cached_func;

    /**
     * contains a map between cbi_name and module data
     */
    std::unordered_map<std::string, CBIData> data;

    /**
     * a map between absolute file paths, and public symbols inside them
     */
    std::unordered_map<std::string, std::unordered_map<std::string, void*>> symbol_maps;

    /**
     * a map between interface names like Lexer, SourceProvider and their actual symbols
     * these symbols correspond to function pointers in the our source code
     */
    std::unordered_map<std::string_view, std::unordered_map<std::string_view, void*>> interface_maps;

    /**
     * diagnostics during compilation of c files
     */
    std::vector<std::string> diagnostics;

    /**
     * path to current executable, resources required by tcc are located relative to it
     */
    std::string exe_path;

    /**
     * constructor
     */
    explicit CompilerBinder(std::string exe_path);

    /**
     * creates a cbi, in which multiple modules can exist
     */
    CBIData* create_cbi(const std::string& name, unsigned int mod_count);

    /**
     * following c program will be compiled as the cbi
     */
    BinderResult compile(
        CBIData& cbiData,
        const std::string& program,
        std::vector<std::string_view>& imports,
        std::vector<std::string_view>& current_files,
        const std::vector<std::string>& compiler_interfaces,
        ASTProcessor& processor
    );

    /**
     * import compiler interface with the given name
     */
    bool import_compiler_interface(const std::string& name, TCCState* state);

    /**
     * provides a pointer to function contained inside cbi
     */
    void* provide_func(const std::string& cbi_name, const std::string& funcName);

    /**
     * a lex function type is searched in the symbols, if not found nullptr is returned
     * otherwise the pointer to the lex function that can compile the tokens is provided
     * @param structName is the container struct for which it was generated
     */
    inline cbi_lex_macro_func provide_lex_macro_func(const std::string& cbiName) {
        return (cbi_lex_macro_func) provide_func(cbiName, "lexMacro");
    }

    /**
     * a lex function type is searched in the symbols, if not found nullptr is returned
     * otherwise the pointer to the lex function that can compile the tokens is provided
     * @param structName is the container struct for which it was generated
     */
    inline cbi_parse_macro_func provide_parse_macro_func(const std::string& cbiName) {
        return (cbi_parse_macro_func) provide_func(cbiName, "parseMacro");
    }

    /**
     * a destructor is used to destruct the TCC state
     */
    ~CompilerBinder();

};