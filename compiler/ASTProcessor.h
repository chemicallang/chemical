// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <utility>

#include "compiler/llvmfwd.h"
#include "ast/structures/Scope.h"
#include "ASTProcessorOptions.h"
#include "ASTDiag.h"
#include "parser/model/CompilerBinder.h"
#include "compiler/lab/LabModule.h"
#include "compiler/lab/LabBuildContext.h"
#include "utils/Benchmark.h"
#include "ast/base/ASTUnit.h"
#include "ast/base/ASTAllocator.h"
#include "integration/cbi/bindings/CBI.h"
#include "integration/common/Diagnostic.h"
#include "cst/LocationManager.h"
#include "compiler/symres/SymbolRange.h"
#include "compiler/processor/ASTFileMetaData.h"
#include "compiler/processor/BuildLabModuleDependency.h"
#include <span>
#include <mutex>

class Parser;

class SymbolResolver;

class ToCAstVisitor;

class ImportPathHandler;

#ifdef COMPILER_BUILD

class CTranslator;

#endif

namespace ctpl {
    class thread_pool;
}

struct ASTFileResult : ASTFileMetaData {

    /**
     * should the processing be continued, this is false, if ast contained errors
     */
    bool continue_processing;

    /**
     * the parsed unit
     */
    ASTUnit unit;

    /**
     * the compile unit
     */
    llvm::DICompileUnit* diCompileUnit = nullptr;

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

    /**
     * constructor
     */
    ASTFileResult(
        unsigned int file_id,
        std::string abs_path,
        LabModule* module
    ) : ASTFileMetaData(file_id, module, std::move(abs_path)), continue_processing(true),
        unit(file_id, chem::string_view(this->abs_path), module ? &module->module_scope : nullptr)
    {

    }

};

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
    ImportPathHandler& path_handler;

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
    std::unordered_map<std::string, ASTFileResult> cache;

    /**
     * the compiler binder that will be used
     */
    CompilerBinder& binder;

    /**
     * the symbol resolver that will resolve all the symbols
     */
    SymbolResolver* resolver;

    /**
     * the lab build context allows us to get any modules user imports files from
     * if not given, it might mean that the build hasn't been invoked on a build.lab
     * TODO: we must figure out a way to allow access modules without the LabBuildContext
     */
    LabBuildContext* context = nullptr;

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
     * NOTE: THIS BOOLEAN IS VERY SCARY, IT MUST BE USED WITH EXTREME CARE
     * when true, all the files parsed will go into job allocator, module allocator won't
     * be used, job allocator will be used to parse the entire files
     * the module allocator frees everything after compilation of the module, only functions
     * that are public are allocated on job allocator, which makes this process super efficient
     *
     * why do we need this ? because build.lab compilation triggers module compilation
     * after compilation of first module from build.lab the module allocator disposes everything
     * and build.lab which was parsed using module allocator also dies, so we can't compile build.lab now
     * we set this boolean to true to keep build.lab on job allocator, while we compile other modules
     *
     * WHEN YOU SET THIS TO TRUE, ALWAYS SET IT BACK TO FALSE, AFTER PARSING A SINGLE FILE
     * WHEN PARSING AN ENTIRE IMPORT TREE CONCURRENTLY (DO NOT USE IT)
     *
     */
    bool parse_on_job_allocator = false;

    /**
     * check if the result has empty diagnostics
     */
    static bool empty_diags(ASTFileResult& result);

    /**
     * print results for the given result
     */
    static void print_results(ASTFileResult& result, const chem::string_view& abs_path, bool benchmark);

    /**
     * constructor
     */
    ASTProcessor(
            ImportPathHandler& pathHandler,
            ASTProcessorOptions* options,
            LabBuildContext* context,
            LocationManager& loc_man,
            SymbolResolver* resolver,
            CompilerBinder& binder,
            ASTAllocator& job_allocator,
            ASTAllocator& mod_allocator,
            ASTAllocator& file_allocator
    ) : loc_man(loc_man), options(options), resolver(resolver), path_handler(pathHandler), binder(binder),
        job_allocator(job_allocator), mod_allocator(mod_allocator), context(context), file_allocator(file_allocator)
    {

    }

    /**
     * the reason we clear the file allocator like this is because maybe a single allocator
     * is being used for file, module and job, this would mean clearing the file allocator
     * would clear things allocated for module or job
     * for example when building the build.lab we use a single allocator
     */
    inline void safe_clear_file_allocator() {
        if(&file_allocator != &mod_allocator && &file_allocator != &job_allocator) {
            file_allocator.clear();
        }
    }

    /**
     * this imports the given files in parallel using the given thread pool
     * @return true if succeeding importing all files with continue_processing, false otherwise
     */
    bool import_chemical_files(
            ctpl::thread_pool& pool,
            std::vector<ASTFileResult*>& out_files,
            std::vector<ASTFileMetaData>& files
    );

    /**
     * determines the files in a module, if it's a directory module, we traverse the directory
     * if it's a files module, we include all the files
     */
    void determine_mod_files(
            std::vector<ASTFileMetaData>& outFiles,
            LabModule* module
    );

    /**
     * imports module files
     */
    bool import_mod_files(
            ctpl::thread_pool& pool,
            std::vector<ASTFileResult*>& out_files,
            std::vector<ASTFileMetaData>& files,
            LabModule* module
    );

    /**
     * figures out direct imports of the given file (fileData), the fileNodes are the nodes
     * that are contained inside the file
     */
    void figure_out_direct_imports(
            ASTFileMetaData& fileData,
            std::vector<ASTNode*>& fileNodes,
            std::vector<ASTFileMetaData>& outImports
    );

    /**
     * the problem with build.labs is that user doesn't tell us which module it depends on, so we figure that out based on its imports
     * this function creates modules on the fly based on the imported file by the importer, it also sets the appropriate dependency in the
     * importer's modules
     */
    void figure_out_module_dependency_based_on_import(
            ASTFileMetaData& imported,
            ASTFileMetaData& importer,
            std::vector<BuildLabModuleDependency>& dependencies
    );

    /**
     * import a single file and all it's imports (in parallel) using the given thread pool
     * @return true if success importing this file and it's imports, false otherwise
     */
    bool import_chemical_file(
            ASTFileResult& result,
            ctpl::thread_pool& pool,
            ASTFileMetaData& fileData
    );

    /**
     * import chemical file with absolute path to it
     * @return true if success importing file, false otherwise
     */
    bool import_chemical_file(ASTFileResult& result, unsigned int fileId, const std::string_view& absolute_path);

    /**
     * import chemical file with absolute path to it
     * @return true if success importing file, false otherwise
     */
    bool import_chemical_mod_file(ASTFileResult& result, unsigned int fileId, const std::string_view& absolute_path);

    /**
     * lex, parse in file and return Scope containing nodes
     * without performing any symbol resolution
     */
    bool import_file(ASTFileResult& result, unsigned int fileId, const std::string_view& absolute_path);

    /**
     * it declares all the symbols inside the file and returns a scope index for the file
     */
    SymbolRange sym_res_tld_declare_file(Scope& scope, const std::string& abs_path);

    /**
     * this function is used to resolve symbols inside the file, the scope_index is used to enable
     * the file's private symbols
     */
    void sym_res_link_file(Scope& scope, const std::string& abs_path, const SymbolRange& range);

    /**
     * all these files would be symbol resolved, 1 is returned in case
     * errors are encountered during symbol resolution of one file
     */
    int sym_res_files(std::vector<ASTFileResult*>& files);

    /**
     * print given benchmark results
     */
    static void print_benchmarks(std::ostream& stream, const std::string_view& TAG, BenchmarkResults* results);

    /**
     * print given benchmark results with file path
     */
    static void print_benchmarks(std::ostream& stream, const std::string_view& TAG, const std::string_view& ABS_PATH, BenchmarkResults* results);

    /**
     * translates given import result to c using visitor
     * doesn't perform symbol resolution
     */
    void declare_before_translation(
            ToCAstVisitor& visitor,
            std::vector<ASTNode*>& import_res,
            const std::string& file
    );

    /**
     * translates given import result to c using visitor
     * doesn't perform symbol resolution
     */
    void translate_after_declaration(
            ToCAstVisitor& visitor,
            std::vector<ASTNode*>& import_res,
            const std::string& file
    );

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
     * declare the nodes in C, this is called
     * when the file has been translated in another module
     * and is being imported in this module for the first time
     */
    void external_declare_in_c(
        ToCAstVisitor& visitor,
        Scope& import_res,
        const std::string& file
    );

    /**
     * implement the external module, this is called
     * to generate any generic instantiations we may have imported
     */
    void external_implement_in_c(
        ToCAstVisitor& visitor,
        Scope& import_res,
        const std::string& file
    );

    /**
     * translates the given module to C
     */
    int translate_module(
        ToCAstVisitor& visitor,
        LabModule* module,
        std::vector<ASTFileResult*>& files
    );

#ifdef COMPILER_BUILD

    /**
     * declare nodes using code generator
     */
    void code_gen_declare(
            Codegen& gen,
            std::vector<ASTNode*>& nodes,
            const std::string_view& abs_path
    );

    /**
     * declare nodes using code generator
     */
    void code_gen_compile(
            Codegen& gen,
            std::vector<ASTNode*>& nodes,
            const std::string_view& abs_path
    );

    /**
     * external implement is called upon nodes that are imported from other modules
     * when importing nodes from other modules, first other module nodes are declared, then
     * current module nodes are declared and then we compile/implement imported nodes
     */
    void code_gen_external_implement(
            Codegen& gen,
            std::vector<ASTNode*>& nodes,
            const std::string_view& abs_path
    );

    /**
     * compile nodes using code generator
     */
    void declare_and_compile(
            Codegen& gen,
            std::vector<ASTNode*>& nodes,
            const std::string_view& abs_path
    );

    /**
     * compile nodes using code generator
     */
    void external_declare_nodes(
        Codegen& gen,
        Scope& import_res,
        const std::string& file
    );

    /**
     * will compile given files for the module, by first declaring all the files and then compiling
     * allowing symbols from files to be referenced in other files above or below
     */
    int compile_module(
        Codegen& gen,
        LabModule* module,
        std::vector<ASTFileResult*>& files
    );

#endif

};