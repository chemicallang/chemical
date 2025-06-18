// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <utility>

#include "ast/structures/Scope.h"
#include "ASTProcessorOptions.h"
#include "ASTDiag.h"
#include "compiler/cbi/model/CompilerBinder.h"
#include "compiler/lab/LabModule.h"
#include "compiler/lab/LabBuildContext.h"
#include "utils/Benchmark.h"
#include "ast/base/ASTUnit.h"
#include "ast/base/ASTAllocator.h"
#include "compiler/cbi/bindings/CBI.h"
#include "core/diag/Diagnostic.h"
#include "core/source/LocationManager.h"
#include "compiler/symres/SymbolRange.h"
#include "compiler/processor/ASTFileResult.h"
#include "compiler/processor/ModuleDependencyRecord.h"
#include "compiler/processor/ModuleFileData.h"
#include "stream/InputSource.h"
#include "stream/FileInputSource.h"
#include <span>
#include <mutex>

class Parser;

class SymbolResolver;

class ToCAstVisitor;

class ImportPathHandler;

class TypeBuilder;

#ifdef COMPILER_BUILD

class CTranslator;

#endif

namespace ctpl {
    class thread_pool;
}

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
     * module storage allows us to check which modules are present in compilation
     * so we can resolve import statements
     */
    ModuleStorage& mod_storage;

    /**
     * import mutex is used to synchronize launching of multiple files
     */
    std::mutex import_mutex;

    /**
     * cache is where files parsed are stored, before parsing the
     * file we search for it in this cache
     */
    std::unordered_map<std::string, std::unique_ptr<ASTFileResult>> cache;

    /**
     * the compiler binder that will be used
     */
    CompilerBinder& binder;

    /**
     * the type builder allows to cache types
     */
    TypeBuilder& type_builder;

    /**
     * the symbol resolver that will resolve all the symbols
     */
    SymbolResolver* resolver;

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
            ModuleStorage& mod_storage,
            LocationManager& loc_man,
            SymbolResolver* resolver,
            CompilerBinder& binder,
            TypeBuilder& typeBuilder,
            ASTAllocator& job_allocator,
            ASTAllocator& mod_allocator,
            ASTAllocator& file_allocator
    ) : loc_man(loc_man), options(options), resolver(resolver), path_handler(pathHandler), binder(binder), type_builder(typeBuilder),
        job_allocator(job_allocator), mod_allocator(mod_allocator), mod_storage(mod_storage), file_allocator(file_allocator)
    {

    }

    /**
     * make a file input source for given path and report error into the given result
     */
    static std::optional<FileInputSource> make_file_input_source(const char* abs_path, ASTFileResult& result);

    /**
     * this imports the given files in parallel using the given thread pool
     * this doesn't handle any import statements present inside the files
     * @return true if succeeding importing all files with continue_processing, false otherwise
     */
    bool import_chemical_files_direct(
            ctpl::thread_pool& pool,
            std::vector<ASTFileMetaData>& files
    );

    /**
     * this imports the given files in parallel using the given thread pool
     * this also imports any files for which import statements are present in the given
     * files, it recursively handles import statements
     * @return true if succeeding importing all files with continue_processing, false otherwise
     */
    bool import_chemical_files_recursive(
            ctpl::thread_pool& pool,
            std::vector<ASTFileResult*>& out_files,
            std::vector<ASTFileMetaData>& files,
            bool use_job_allocator
    );

    /**
     * determine the direct files in the module, for example if this is a directory module
     * we traverse the whole directory and it's nested folders to determine direct files
     * this doesn't take into account import statements that import files that aren't present
     * inside the module
     */
     static void determine_module_files(
             ImportPathHandler& handler,
             LocationManager& locMan,
             LabModule* module
     );

    /**
     * helper method to determine the direct files in the module
     */
    inline void determine_module_files(LabModule* module) {
        determine_module_files(path_handler, loc_man, module);
    }

    /**
     * imports files in a module, this just lexes and parses the file, no symbol resolution
     * the given 'files' are lexed and parsed into units which are put into 'out_files'
     */
    bool import_module_files_direct(
            ctpl::thread_pool& pool,
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
            ASTFileResult& imported,
            std::vector<ModuleDependencyRecord>& dependencies
    );

    /**
     * imports the given mod file at path @param modFile into the @param result
     * this translates the mod file into a build.lab before importing it
     */
    bool import_mod_file_as_lab(
            ASTFileMetaData& meta,
            ASTFileResult& result,
            bool use_job_allocator,
            InputSource* inp_source
    );

    /**
     * imports the given mod file at path @param modFile into the @param result
     * this translates the mod file into a build.lab before importing it
     */
    bool import_mod_file_as_lab(
            ASTFileMetaData& meta,
            ASTFileResult& result,
            bool use_job_allocator
    ) {
        auto inp_source = make_file_input_source(meta.abs_path.data(), result);
        if(inp_source.has_value()) {
            return import_mod_file_as_lab(meta, result, use_job_allocator, &inp_source.value());
        } else {
            return false;
        }
    }

    /**
     * import a single file and all it's imports (in parallel) using the given thread pool
     * @return true if success importing this file and it's imports, false otherwise
     */
    bool import_chemical_file_recursive(
            ASTFileResult& result,
            ctpl::thread_pool& pool,
            ASTFileMetaData& fileData,
            bool use_job_allocator
    );

    /**
     * import chemical file with absolute path to it
     * @return true if success importing file, false otherwise
     */
    bool import_chemical_file(
            ASTFileResult& result,
            unsigned int fileId,
            const std::string_view& absolute_path,
            InputSource* source,
            bool use_job_allocator
    );

    /**
     * import chemical file with absolute path to it
     * @return true if success importing file, false otherwise
     */
    bool import_chemical_file(
            ASTFileResult& result,
            unsigned int fileId,
            const std::string_view& absolute_path,
            bool use_job_allocator
    ) {
        auto inp_source = make_file_input_source(absolute_path.data(), result);
        if(inp_source.has_value()) {
            return import_chemical_file(result, fileId, absolute_path, &inp_source.value(), use_job_allocator);
        } else {
            return false;
        }
    }

    /**
     * lex, parse in file and return Scope containing nodes
     * without performing any symbol resolution
     */
    bool import_file(
            ASTFileResult& result,
            unsigned int fileId,
            const std::string_view& abs_path,
            bool use_job_allocator
    ) {
        return import_chemical_file(result, fileId, abs_path, use_job_allocator);
    }

    /**
     * import chemical file with absolute path to it
     * @return true if success importing file, false otherwise
     */
    static bool import_chemical_mod_file(
            ASTAllocator& fileAllocator,
            ASTAllocator& modAllocator,
            LocationManager& loc_man,
            ModuleFileData& data,
            unsigned int fileId,
            const std::string_view& absolute_path
    );

    /**
     * import chemical file with absolute path to it
     * @return true if success importing file, false otherwise
     */
    static bool import_chemical_mod_file(
            ASTAllocator& fileAllocator,
            ASTAllocator& modAllocator,
            LocationManager& loc_man,
            ModuleFileData& data,
            unsigned int fileId,
            const std::string_view& abs_path,
            InputSource* inp_source
    );

    /**
     * it declares all the symbols inside the file and returns a scope index for the file
     */
    SymbolRange sym_res_tld_declare_file(
            Scope& scope,
            unsigned int fileId,
            const std::string& abs_path
    );

    /**
     * links the signature of the file
     */
    void sym_res_link_sig_file(
            Scope& scope,
            unsigned int fileId,
            const std::string& abs_path,
            const SymbolRange& range
    );

    /**
     * this function is used to resolve symbols inside the file, the scope_index is used to enable
     * the file's private symbols
     */
    void sym_res_link_file(
            Scope& scope,
            unsigned int fileId,
            const std::string& abs_path,
            const SymbolRange& range
    );

    /**
     * declare and link file in one shot, no symbols of the file will be retained
     */
    void sym_res_declare_and_link_file(
            Scope& scope,
            unsigned int fileId,
            const std::string& abs_path
    );

    /**
     * symbol resolves the module, creating the scope
     */
    int sym_res_module(LabModule* module);

    /**
     * this symbol resolves the module, however sequentially, which means
     * symbols between files aren't shared, unless imported using as_identifier explicitly
     * this prevents conflict between files when they are being compiled in a single
     * module but don't belong to it
     */
    int sym_res_module_seq(LabModule* module);

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
        LabModule* module
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
        LabModule* module
    );

#endif

};

void shallow_dedupe_sorted(std::vector<LabModule*>& outMods, std::vector<LabModule*>& inMods);