// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "CBIData.h"
#include "compiler/cbi/model/Model.h"
#include <unordered_map>
#include "std/chem_string_view.h"
#include "compiler/cbi/bindings/CBI.h"
#include "compiler/lab/CBIType.h"
#include "CBIFunctionIndex.h"

class ASTProcessor;

class Parser;

class CSTToken;

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
     * parseMacroNode functions are called by the parser to provide a node for a given macro
     * when parsing
     */
    std::unordered_map<chem::string_view, UserParserParseMacroNodeFn> parseMacroNodeFunctions;

    /**
     * contains a map between cbi_name and module data
     */
    std::unordered_map<std::string, CBIData> data;

    /**
     * a map between interface names like Lexer, SourceProvider and their actual symbols
     * these symbols correspond to function pointers in the our source code
     */
    std::unordered_map<chem::string_view, std::span<const std::pair<chem::string_view, void*>>> interface_maps;

    /**
     * path to current executable, resources required by tcc are located relative to it
     */
    std::string exe_path;

    /**
     * constructor
     */
    explicit CompilerBinder(std::string exe_path);

    /**
     * imports the given compiler interfaces
     */
    static void import_compiler_interface(const std::span<const std::pair<chem::string_view, void*>>& interface, TCCState* state);

    /**
     * a macro lexer is prepared from the given tcc state
     */
    const char* prepare_macro_lexer_from(const chem::string_view& cbiName, TCCState* state);

    /**
     * a macro parser is prepared from the given tcc state
     */
    const char* prepare_macro_parser_from(const chem::string_view& cbiName, TCCState* state);

    /**
     * prepare the cbi type with given state
     */
    const char* prepare_with_type(const chem::string_view& cbiName, TCCState* state, CBIType type);

    /**
     * indexes the given function
     */
    const char* index_function(CBIFunctionIndex& index, TCCState* state);

    /**
     * a destructor is used to destruct the TCC state
     */
    ~CompilerBinder();

};