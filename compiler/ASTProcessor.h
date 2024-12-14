// Copyright (c) Qinetik 2024.

#pragma once

#include <utility>

#include "ast/structures/Scope.h"
#include "ASTProcessorOptions.h"
#include "preprocess/ImportGraphMaker.h"
#include "ASTDiag.h"
#include "parser/model/CompilerBinder.h"
#include "compiler/lab/LabModule.h"
#include "compiler/lab/LabBuildContext.h"
#include "utils/Benchmark.h"
#include "ast/base/ASTUnit.h"
#include "cst/base/CSTUnit.h"
#include "ast/base/ASTAllocator.h"
#include "integration/cbi/bindings/CBI.h"
#include <span>
#include <mutex>

class Parser;

class CSTConverter;

class SymbolResolver;

class ShrinkingVisitor;

class ToCAstVisitor;

#ifdef COMPILER_BUILD

class CTranslator;

#endif

namespace ctpl {
    class thread_pool;
}

struct ASTFileResultData {

    /**
     * should the processing be continued, this is false, if ast contained errors
     */
    bool continue_processing;

    /**
     * if this unit belongs to a c file that was translated using clang
     */
    bool is_c_file;

};

struct ASTFileMetaData {

    /**
     * the id of the file
     */
    unsigned int file_id;

    /**
     * the path used when user imported the file
     */
    std::string import_path;

    /**
     * the absolute path determined to the file
     */
    std::string abs_path;

    /**
     * the as identifier is used with import statements to import files
     */
    std::string as_identifier;

};


struct ASTFileResult : ASTFileResultData, ASTFileMetaData {

    /**
     * the parsed unit
     */
    ASTUnit unit;

    /**
     * the imported files by this file, these files don't contain duplicates
     * or already imported files
     */
    std::vector<ASTFileResult*> imports;

    /**
     * if read error occurred this would contain it
     */
    std::string read_error;

    /**
     * diagnotics collected during the lexing process
     */
    std::vector<Diag> lex_diagnostics;

    /**
     * diagnostics collected during the conversion process
     * These diagnostics will be translation diagnostics, if it's a c file
     */
    std::vector<Diag> parse_diagnostics;

    /**
     * the benchmark results are stored here, if user opted for benchmarking
     */
    std::unique_ptr<BenchmarkResults> lex_benchmark;

    /**
     * the parsing benchmarks are stored here, if user opted for benchmarking
     * This will be c translation benchmarks, if it's c file
     */
    std::unique_ptr<BenchmarkResults> parse_benchmark;

};

/**
 * @deprecated
 */
using ASTFileResultNew = ASTFileResult;

/**
 * this will be called ASTProcessor
 */
class ASTProcessor {
public:

    /**
     * the location manager
     */
    LocationManager& loc_man;

    /**
     * processor options
     */
    ASTProcessorOptions* options;

    /**
     * import path handler, handles paths, '@' symbols in paths, determining their absolute paths
     */
    ImportPathHandler path_handler;

    /**
     * import mutex is used to synchronize launching of multiple files
     */
    std::mutex import_mutex;

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
     * cache is where files parsed are stored, before parsing the
     * file we search for it in this cache
     */
    std::unordered_map<std::string, ASTFileResultNew> cache;

    /**
     * the compiler binder that will be used
     */
    CompilerBinder& binder;

    /**
     * the symbol resolver that will resolve all the symbols
     */
    SymbolResolver* resolver;

#ifdef COMPILER_BUILD

    /**
     * translator is used to translate c files during import
     */
    CTranslator* const translator;

#endif

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
     * File level allocator
     */
    ASTAllocator& file_allocator;

    /**
     * constructor
     */
    ASTProcessor(
            ASTProcessorOptions* options,
            LocationManager& loc_man,
            SymbolResolver* resolver,
            CompilerBinder& binder,
#ifdef COMPILER_BUILD
            CTranslator* translator,
#endif
            ASTAllocator& job_allocator,
            ASTAllocator& mod_allocator,
            ASTAllocator& file_allocator
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
     * this imports the given files in parallel using the given thread pool
     */
    void import_chemical_files(
            ctpl::thread_pool& pool,
            std::vector<ASTFileResultNew*>& out_files,
            std::vector<ASTFileMetaData>& files
    );

    /**
     * determine module imports
     */
    void determine_mod_imports(
            ctpl::thread_pool& pool,
            std::vector<ASTFileResultNew*>& out_files,
            LabModule* module
    );

    /**
     * import a single file and all it's imports (in parallel) using the given thread pool
     */
    void import_chemical_file(
            ASTFileResultNew& result,
            ctpl::thread_pool& pool,
            ASTFileMetaData& fileData
    );

    /**
     * import chemical file with absolute path to it
     */
    void import_chemical_file(ASTFileResultNew& result, unsigned int fileId, const std::string_view& absolute_path);

    /**
     * lex, parse in file and return Scope containing nodes
     * without performing any symbol resolution
     */
    void import_file(ASTFileResultNew& result, unsigned int fileId, const std::string_view& absolute_path);

    /**
     * function that performs symbol resolution
     */
    void sym_res(Scope& scope, bool is_c_file, const std::string& abs_path);

    /**
     * print given benchmark results
     */
    static void print_benchmarks(std::ostream& stream, const std::string& TAG, BenchmarkResults* results);

    /**
     * translates given import result to c using visitor
     * doesn't perform symbol resolution
     */
    void translate_to_c(
        ToCAstVisitor& visitor,
        std::vector<ASTNode*>& import_res,
        const std::string& file
    );

    /**
     * translates given import result to c using visitor
     * doesn't perform symbol resolution
     */
    inline void translate_to_c(
            ToCAstVisitor& visitor,
            Scope& import_res,
            const FlatIGFile& file
    ) {
        translate_to_c(visitor, import_res.nodes, file.abs_path);
    }

    /**
     * declare the nodes in C, this is called
     * when the file has been translated in another module
     * and is being imported in this module for the first time
     */
    void declare_in_c(
        ToCAstVisitor& visitor,
        Scope& import_res,
        const std::string& file
    );

    /**
     * shrink the imported nodes
     */
    void shrink_nodes(
        ShrinkingVisitor& visitor,
        ASTUnit unit,
        const std::string& file
    );

#ifdef COMPILER_BUILD

    /**
     * compile nodes using code generator
     */
    void compile_nodes(
            Codegen& gen,
            std::vector<ASTNode*>& nodes,
            const std::string_view& abs_path
    );

    /**
     * compile nodes using code generator
     */
    inline void compile_nodes(
        Codegen& gen,
        Scope& import_res,
        const FlatIGFile &file
    ) {
        compile_nodes(gen, import_res.nodes, file.abs_path);
    }

    /**
     * compile nodes using code generator
     */
    void declare_nodes(
        Codegen& gen,
        Scope& import_res,
        const std::string& file
    );

#endif

};