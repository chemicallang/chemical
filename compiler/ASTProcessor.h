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
#include "utils/Benchmark.h"
#include "ast/base/ASTUnit.h"
#include "cst/base/CSTUnit.h"
#include "ast/base/ASTAllocator.h"
#include "integration/cbi/bindings/CBI.h"

class Lexer;

class CSTConverter;

class SymbolResolver;

class ShrinkingVisitor;

class ToCAstVisitor;

/**
 * when a file is processed using ASTProcessor, it results in this result
 */
struct ASTImportResult {

    ASTUnit unit;
    CSTUnit cst_unit;
    bool continue_processing;
    bool is_c_file;

};

struct ASTImportResultExt : ASTImportResult {

    /**
     * this field provides cli_out, this is here
     * because a file is imported concurrently, since multiple
     * files are compiled, we use this field to put everything we want to print
     * for that file into this variable and then print it
     */
    std::string cli_out;

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
     * import path handler, handles paths, '@' symbols in paths, determining their absolute paths
     */
    ImportPathHandler path_handler;

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
    std::unordered_map<std::string, ASTUnit> shrinked_unit;

    /**
     * the compiler binder that will be used
     */
    CompilerBinder& binder;

    /**
     * the symbol resolver that will resolve all the symbols
     */
    SymbolResolver* resolver;

    /**
     * it's a container of AST diagnostics
     * this is here because c file's errors are ignored because they contain unresolvable symbols
     */
    std::vector<Diag> previous;

    /**
     * Job (executable or dll) level allocator
     */
    ASTAllocator& job_allocator;

    /**
     * Module level allocator
     */
    ASTAllocator& mod_allocator;

    /**
     * constructor
     */
    ASTProcessor(
            ASTProcessorOptions* options,
            SymbolResolver* resolver,
            CompilerBinder& binder,
            ASTAllocator& job_allocator,
            ASTAllocator& mod_allocator
    );

    /**
     * this allows to convert more than one path and get flat imports
     */
    std::vector<FlatIGFile> flat_imports_mul(const std::vector<std::string>& paths);

    /**
     * get flat imports
     */
    std::vector<FlatIGFile> flat_imports(const std::string& path) {
        return flat_imports_mul({ path });
    }

    /**
     * will determine the source files required by this module, in order
     * they should be compiled
     */
    std::vector<FlatIGFile> determine_mod_imports(LabModule* module);

    /**
     * lex, parse and resolve symbols in file and return Scope containing nodes
     * without performing any symbol resolution
     */
    ASTImportResultExt import_file(const FlatIGFile& file);

    /**
     * function that performs symbol resolution
     */
    void sym_res(Scope& scope, bool is_c_file, const std::string& abs_path);

    /**
     * print given benchmark results
     */
    void print_benchmarks(std::ostream& stream, const std::string& TAG, BenchmarkResults* results);

    /**
     * translates given import result to c using visitor
     * doesn't perform symbol resolution
     */
    void translate_to_c(
        ToCAstVisitor& visitor,
        Scope& import_res,
        const FlatIGFile& file
    );

    /**
     * declare the nodes in C, this is called
     * when the file has been translated in another module
     * and is being imported in this module for the first time
     */
    void declare_in_c(
        ToCAstVisitor& visitor,
        Scope& import_res,
        const FlatIGFile& file
    );

    /**
     * shrink the imported nodes
     */
    void shrink_nodes(
        ShrinkingVisitor& visitor,
        ASTUnit unit,
        const FlatIGFile& file
    );

};

/**
 * this function can be called concurrently, to import files
 */
ASTImportResultExt concurrent_processor(int id, int job_id, const FlatIGFile& file, ASTProcessor* processor);