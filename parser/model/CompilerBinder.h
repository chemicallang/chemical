// Copyright (c) Qinetik 2024.

#pragma once

#include "CBIData.h"
#include "compiler/cbi/Model.h"
#include <unordered_map>
#include "std/chem_string_view.h"

class ASTProcessor;

class Parser;

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
 * compiler binder based on tiny c compiler
 */
class CompilerBinder {
public:

    /**
     * all the initializer functions are indexed in the single unordered map
     */
    std::unordered_map<chem::string_view, UserLexerInitializeFn> initializeLexerFunctions;

    /**
     * parseMacroValue functions are called by the parser to provide a value for a given macro
     * when parsing
     */
    std::unordered_map<chem::string_view, UserParserParseMacroValueFn> parseMacroValueFunctions;

    /**
     * contains a map between cbi_name and module data
     */
    std::unordered_map<std::string, CBIData> data;

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
        const std::vector<std::string>& compiler_interfaces,
        ASTProcessor& processor
    );

    /**
     * import compiler interface with the given name
     */
    bool import_compiler_interface(const std::string& name, TCCState* state);

    /**
     * a destructor is used to destruct the TCC state
     */
    ~CompilerBinder();

};