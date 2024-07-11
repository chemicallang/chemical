// Copyright (c) Qinetik 2024.

#pragma once

#include <utility>

#include "ast/structures/Scope.h"
#include "ASTProcessorOptions.h"
#include "preprocess/ImportGraphMaker.h"
#include "ASTDiag.h"
#include "lexer/model/CompilerBinder.h"
#include "compiler/lab/LabModule.h"
#include "compiler/lab/LabBuildContext.h"

class Lexer;

class CSTConverter;

class SymbolResolver;

class ShrinkingVisitor;

class ToCAstVisitor;

/**
 * when a file is processed using ASTProcessor, it results in this result
 */
struct ASTImportResult {

    Scope scope;
    bool continue_processing;
    bool is_c_file;

};

/**
 * this will be called ASTProcessor
 */
class ASTProcessor {
public:

    /**
     * processor options
     */
    ASTProcessorOptions* options;

    /**
     * When a file is processed, we shrink it's nodes (remove function bodies) using the
     * ShrinkingVisitor and then keep them on this map, with absolute path to files as keys
     *
     * Now when user has a module that depends on another module, We don't want to waste memory
     * We first process the first module, each file's nodes are shrinked and put on this map
     *
     * When user imports a file, in the other module, this map is checked to see if that file has been
     * already processed (generated code for), if it has, this map contains function signatures alive and well
     * we declare those functions in this module basically importing them
     *
     * This Allows caching files in a module that have been processed to be imported by other modules that depend on it
     *
     */
    std::unordered_map<std::string, std::vector<std::unique_ptr<ASTNode>>> shrinked_nodes;

    /**
     * compiler binder that will be used through out processing
     */
    std::unique_ptr<CompilerBinder> binder;

    /**
     * the lexer cbi, that is initialized if cbi enabled
     * passed to lexer cbi
     */
    std::unique_ptr<LexerCBI> lexer_cbi;

    /**
     * the source provider cbi, that is initialized if cbi enabled
     * passed to lexer cbi
     */
    std::unique_ptr<SourceProviderCBI> provider_cbi;

    /**
     * the symbol resolver that will resolve all the symbols
     */
    SymbolResolver* resolver;

    /**
     * it's a container of AST diagnostics
     * this is here because c file's errors are ignored because they contain unresolvable symbols
     */
    std::vector<ASTDiag> previous;

    /**
     * constructor
     */
    ASTProcessor(
            ASTProcessorOptions* options,
            SymbolResolver* resolver
    );

    /**
     * get flat imports
     */
    std::vector<FlatIGFile> flat_imports(const std::string& path);

    /**
     * get module for the given file, the returned module depends upon the file extension
     */
    LabModule* get_root_module(LabBuildContext& context, const std::string& path);

    /**
     * will determine the source files required by this module, in order
     * they should be compiled
     */
    std::vector<FlatIGFile> determine_mod_imports(LabModule* module);

    /**
     * lex, parse and resolve symbols in file and return Scope containing nodes
     * without performing any symbol resolution
     */
    ASTImportResult import_file(const FlatIGFile& file);

    /**
     * function that performs symbol resolution
     */
    void sym_res(Scope& scope, bool is_c_file, const std::string& abs_path);

    /**
     * translates given import result to c using visitor
     * doesn't perform symbol resolution
     */
    void translate_to_c_no_sym_res(
        ToCAstVisitor& visitor,
        Scope& import_res,
        ShrinkingVisitor& shrinker,
        const FlatIGFile& file
    );


    /**
     * translates given import result (retrieved via import_file) to c using visitor
     * @return true if success, false if failure (returns early on failure)
     */
    bool translate_to_c(
        ToCAstVisitor& visitor,
        ASTImportResult& import_res,
        ShrinkingVisitor& shrinker,
        const FlatIGFile& file
    );

    /**
     * called when all files are done
     */
    void end();

};

/**
 * this function can be called concurrently, to import files
 */
ASTImportResult concurrent_processor(int id, int job_id, const FlatIGFile& file, ASTProcessor* processor);