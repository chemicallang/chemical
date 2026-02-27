// Copyright (c) Chemical Language Foundation 2025.

#include "ASTProcessor.h"

#include <memory>
#include "parser/Parser.h"
#include "compiler/SymbolResolver.h"
#include "compiler/typeverify/TypeVerifyAPI.h"
#include "preprocess/2c/2cASTVisitor.h"
#include "utils/Benchmark.h"
#include <sstream>
#include "utils/PathUtils.h"
#include "compiler/lab/LabBuildCompiler.h"
#include "std/chem_string.h"
#include "rang.hpp"
#include "preprocess/RepresentationVisitor.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/statements/ChildrenMapNode.h"
#include "ast/statements/VarInit.h"
#include <filesystem>
#include "lexer/Lexer.h"
#include "stream/FileInputSource.h"
#include "ast/base/GlobalInterpretScope.h"
#include "preprocess/ImportPathHandler.h"
#include "compiler/lab/mod_conv/ModToLabConverter.h"
#include "ast/statements/Import.h"
#include "compiler/symres/NodeSymbolDeclarer.h"
#include "compiler/lab/LabGetMethodInjection.h"

#ifdef COMPILER_BUILD
#include "compiler/ctranslator/CTranslator.h"
#endif

std::string ASTProcessorOptions::get_resources_path() {
    if(!resources_path.empty()) return resources_path;
    resources_path = resources_path_rel_to_exe(exe_path);
    if(resources_path.empty()) {
        std::cerr << rang::fg::yellow << "warning: " << rang::fg::reset;
        std::cerr << "couldn't locate resources path relative to compiler's executable" << std::endl;
    }
    return resources_path;
}

void getFilesInDirectory(std::vector<std::string>& filePaths, const std::filesystem::path& dirPath) {
    for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
        if (entry.is_regular_file()) {
            const auto fPath = entry.path().string();
            if(fPath.ends_with(".ch")) {
                filePaths.emplace_back(canonical_path(fPath));
                if (filePaths.back().empty()) {
                    // will not happen most of the time, since OS is providing us the paths
                    std::cerr << "error: couldn't determine canonical path for the file '" << entry.path().string() << '\'' << std::endl;
                    filePaths.pop_back();
                }
            }
        } else if(entry.is_directory()) {
            getFilesInDirectory(filePaths, entry.path());
        }
    }
}

bool ASTProcessor::empty_diags(ASTFileResult& result) {
    return result.lex_diagnostics.empty() && result.parse_diagnostics.empty();
}

void ASTProcessor::print_results(ASTFileResult& result, const chem::string_view& abs_path, bool benchmark) {
    if(!empty_diags(result)) {
        std::cout << rang::style::bold << rang::fg::magenta << "[Parsed] " << abs_path << rang::fg::reset << rang::style::reset << '\n';
        Diagnoser::print_diagnostics(result.lex_diagnostics, abs_path, "Lexer");
        Diagnoser::print_diagnostics(result.parse_diagnostics, abs_path, "Parser");
    }
    if (benchmark) {
        ASTProcessor::print_benchmarks(std::cout, "Lexer", &result.lex_benchmark);
        ASTProcessor::print_benchmarks(std::cout, "Parser", &result.parse_benchmark);
    }
    std::cout << std::flush;
}

void ASTProcessor::determine_module_files(
        ImportPathHandler& path_handler,
        LocationManager& loc_man,
        LabModule* module
) {
    auto& files = module->direct_files;
    if(!files.empty()) {
        // it seems the files have already been determined
        return;
    }
    switch(module->type) {
        case LabModuleType::Files: {
            for (auto& str: module->paths) {
                auto abs_path = canonical(str.to_view());
                if (abs_path.empty()) {
                    std::cerr << rang::fg::red << "error: " << rang::fg::reset << "couldn't determine canonical path for file '" << str.data() << "' in module '" << module->name << '\'' << std::endl;
                    continue;
                }
                auto fileId = loc_man.encodeFile(abs_path);
                // all these files belong to the given module, so it's scope will be used
                files.emplace_back(fileId, &module->module_scope, std::move(abs_path), nullptr);
            }
            return;
        }
        case LabModuleType::ObjFile:
        case LabModuleType::CFile:
        case LabModuleType::CPPFile:
            return;
        case LabModuleType::Directory:
            for(auto& dir_path : module->paths) {
                auto dir_path_p = (std::filesystem::path) dir_path.to_view();
                if (!std::filesystem::exists(dir_path_p)) {
                    std::cerr << rang::fg::red << "error: " << rang::fg::reset << "directory / file doesn't exist at path'" << dir_path << "' in module '" << *module << '\'' << std::endl;
                    continue;
                }
                if (std::filesystem::is_directory(dir_path_p)) {
                    std::vector<std::string> filePaths;
                    getFilesInDirectory(filePaths, dir_path_p);
                    for (auto& abs_path : filePaths) {
                        auto fileId = loc_man.encodeFile(abs_path);
                        files.emplace_back(fileId, &module->module_scope, abs_path, nullptr);
                    }
                } else if(dir_path.ends_with(".ch")) {
                    auto dir_path_view = dir_path.to_view();
                    auto abs_path = canonical_path(dir_path_view);
                    if(abs_path.empty()) {
                        std::cerr << rang::fg::red << "error: " << rang::fg::reset << "couldn't determine canonical path for file '" << dir_path_view << "' in module '" << *module << '\'' << std::endl;
                        continue;
                    } else {
                        auto fileId = loc_man.encodeFile(abs_path);
                        files.emplace_back(fileId, &module->module_scope, abs_path, nullptr);
                    }
                } else {
                    std::cerr << rang::fg::red << "error: " << rang::fg::reset << "path '" << dir_path << "' in module '" << *module << "' is not a directory or a chemical file" << std::endl;
                }
            }
            return;
    }
}

bool ASTProcessor::import_module_files_direct(
        ctpl::thread_pool& pool,
        std::vector<ASTFileMetaData>& files,
        LabModule* module
) {
    return import_chemical_files_direct(pool, files);
}

SymbolRange ASTProcessor::sym_res_tld_declare_file(Scope& scope, unsigned int fileId, const std::string& abs_path) {
    // doing stuff
    auto prev_has_errors = resolver->has_errors;
    BenchmarkResults bm_results;
    if(options->benchmark_files) {
        bm_results.benchmark_begin();
    }
    const auto range = resolver->tld_declare_file(scope, fileId, abs_path);
    if(options->benchmark_files) {
        bm_results.benchmark_end();
        print_benchmarks(std::cout, "SymRes:declare", abs_path, &bm_results);
    }
    if(!resolver->diagnostics.empty()) {
        resolver->print_diagnostics(chem::string_view(abs_path), "SymRes:declare");
    }
    return range;
}

void ASTProcessor::sym_res_before_link_sig_file(
        Scope& scope,
        unsigned int fileId,
        const std::string& abs_path,
        const SymbolRange& range
) {
    // doing stuff
    auto prev_has_errors = resolver->has_errors;
    BenchmarkResults bm_results;
    if(options->benchmark_files) {
        bm_results.benchmark_begin();
    }
    resolver->before_link_signature_file(scope, fileId, range);
    if(options->benchmark_files) {
        bm_results.benchmark_end();
        print_benchmarks(std::cout, "SymRes:before_link_sig", abs_path, &bm_results);
    }
    if(!resolver->diagnostics.empty()) {
        resolver->print_diagnostics(chem::string_view(abs_path), "SymRes:before_link_sig");
    }
}

void ASTProcessor::sym_res_link_sig_file(Scope& scope, unsigned int fileId, const std::string& abs_path, const SymbolRange& range) {
    // doing stuff
    auto prev_has_errors = resolver->has_errors;
    BenchmarkResults bm_results;
    if(options->benchmark_files) {
        bm_results.benchmark_begin();
    }
    resolver->link_signature_file(scope, fileId, range);
    if(options->benchmark_files) {
        bm_results.benchmark_end();
        print_benchmarks(std::cout, "SymRes:link_sig", abs_path, &bm_results);
    }
    if(!resolver->diagnostics.empty()) {
        resolver->print_diagnostics(chem::string_view(abs_path), "SymRes:link_sig");
    }
}

void ASTProcessor::sym_res_after_link_sig_file(
        Scope& scope,
        unsigned int fileId,
        const std::string& abs_path,
        const SymbolRange& range
) {
    // doing stuff
    auto prev_has_errors = resolver->has_errors;
    BenchmarkResults bm_results;
    if(options->benchmark_files) {
        bm_results.benchmark_begin();
    }
    resolver->after_link_signature_file(scope, fileId, range);
    if(options->benchmark_files) {
        bm_results.benchmark_end();
        print_benchmarks(std::cout, "SymRes:after_link_sig", abs_path, &bm_results);
    }
    if(!resolver->diagnostics.empty()) {
        resolver->print_diagnostics(chem::string_view(abs_path), "SymRes:after_link_sig");
    }
}

void ASTProcessor::sym_res_link_file(Scope& scope, unsigned int fileId, const std::string& abs_path, const SymbolRange& range) {
    // doing stuff
    auto prev_has_errors = resolver->has_errors;
    BenchmarkResults bm_results;
    if(options->benchmark_files) {
        bm_results.benchmark_begin();
    }
    resolver->link_file(scope, fileId, range);
    if(options->benchmark_files) {
        bm_results.benchmark_end();
        print_benchmarks(std::cout, "SymRes:link", abs_path, &bm_results);
    }
    if(!resolver->diagnostics.empty()) {
        resolver->print_diagnostics(chem::string_view(abs_path), "SymRes:link");
    }
}

void ASTProcessor::sym_res_declare_and_link_file(Scope& scope, unsigned int fileId, const std::string& abs_path) {
    // doing stuff
    auto prev_has_errors = resolver->has_errors;
    BenchmarkResults bm_results;
    if(options->benchmark_files) {
        bm_results.benchmark_begin();
    }
    resolver->declare_and_link_file(scope, fileId, abs_path);
    if(options->benchmark_files) {
        bm_results.benchmark_end();
        print_benchmarks(std::cout, "SymRes:link_seq", abs_path, &bm_results);
    }
    if(!resolver->diagnostics.empty()) {
        resolver->print_diagnostics(chem::string_view(abs_path), "SymRes:link_seq");
    }
}

inline void declareAllSymbols(SymbolResolver& resolver, ChildrenMapNode* children) {
    // user didn't give any alias or symbols
    // declare everything
    for (auto& sym: children->symbols) {
        resolver.declare_or_shadow(sym.first, sym.second);
    }
}

static void declareChildren(SymbolResolver& resolver, ModuleDependency& dep, ChildrenMapNode* children) {
    if(dep.info == nullptr) {
        declareAllSymbols(resolver, children);
        return;
    }
    if(!dep.info->alias.empty()) {
        // does not shadow
        resolver.declare(dep.info->alias, children);
    } else if(!dep.info->symbols.empty()) {
        // declare imported symbols only
        for(auto& sym : dep.info->symbols) {
            resolver.declareImportedSymbol(children, sym.parts, sym.alias, dep.info->location);
        }
    } else {
        declareAllSymbols(resolver, children);
    }
}

int ASTProcessor::sym_res_module(LabModule* module) {

    const auto prev_mod_scope = resolver->current_mod_scope;
    resolver->current_mod_scope = &module->module_scope;
    const auto mod_index = resolver->module_scope_start();

    // declare symbols of directly imported modules
    for(auto& dep : module->dependencies) {

        // checking if already cached
        if(dep.module->children != nullptr) {
            declareChildren(*resolver, dep, dep.module->children);
            continue;
        }

        // creating new children map node
        const auto children = resolver->ast_allocator->allocate<ChildrenMapNode>();
        new (children) ChildrenMapNode(&module->module_scope, module->module_scope.encoded_location());

        // traversing the dependencies to store children into the map
        MapSymbolDeclarer declarer(children->symbols);
        for(auto& file_ptr : dep.module->direct_files) {
            auto& file = *file_ptr.result;
            for(const auto node : file.unit.scope.body.nodes) {
                declare_node(declarer, node, AccessSpecifier::Public);
            }
        }

        // storing the children for caching
        dep.module->children = children;

        // declare children
        declareChildren(*resolver, dep, children);

    }

    // get symbol table
    auto& table = resolver->getSymbolTable();
    auto& symbols = table.get_symbols();
    const auto module_symbols_start = symbols.size();

    // tiny flag for checking error
    bool errored = false;

    // declare symbols for all files once in the module
    for(auto& file_ptr : module->direct_files) {

        auto& file = *file_ptr.result;

        file.private_symbol_range = sym_res_tld_declare_file(file.unit.scope.body, file.file_id, file.abs_path);

        // report and clear diagnostics
        if (resolver->has_errors && !options->ignore_errors) {
            if(options->stop_on_file_error) return 1;
            errored = true;
        }
        resolver->reset_errors();

    }

    if(errored) return 1;

    // before link the signature of the files
    for(auto& file_ptr : module->direct_files) {

        auto& file = *file_ptr.result;

        sym_res_before_link_sig_file(file.unit.scope.body, file.file_id, file.abs_path, file.private_symbol_range);
        // report and clear diagnostics
        if (resolver->has_errors && !options->ignore_errors) {
            if(options->stop_on_file_error) return 1;
            errored = true;
        }
        resolver->reset_errors();

    }

    if(errored) return 1;

    // link the signature of the files
    for(auto& file_ptr : module->direct_files) {

        auto& file = *file_ptr.result;

        sym_res_link_sig_file(file.unit.scope.body, file.file_id, file.abs_path, file.private_symbol_range);
        // report and clear diagnostics
        if (resolver->has_errors && !options->ignore_errors) {
            if(options->stop_on_file_error) return 1;
            errored = true;
        }
        resolver->reset_errors();

    }

    if(errored) return 1;

    // after link the signature of the files
    for(auto& file_ptr : module->direct_files) {

        auto& file = *file_ptr.result;

        sym_res_after_link_sig_file(file.unit.scope.body, file.file_id, file.abs_path, file.private_symbol_range);
        // report and clear diagnostics
        if (resolver->has_errors && !options->ignore_errors) {
            if(options->stop_on_file_error) return 1;
            errored = true;
        }
        resolver->reset_errors();

    }

    if(errored) return 1;

    // sequentially symbol resolve all the files in the module
    for(auto& file_ptr : module->direct_files) {

        auto& file = *file_ptr.result;

        sym_res_link_file(file.unit.scope.body, file.file_id, file.abs_path, file.private_symbol_range);
        if (resolver->has_errors && !options->ignore_errors) {
            if(options->stop_on_file_error) return 1;
            errored = true;
        }
        resolver->reset_errors();

        // clear everything allocated during symbol resolution of current file
        file_allocator.clear();

    }

    if(errored) return 1;

    // we need to search for main function in each module and make it no_mangle
    // so it won't be mangled (module scope and name gets added, which can cause no entry point error)
    // only do this for application modules - library modules keep main mangled
    if(module->package_kind == PackageKind::Application) {
        const auto main_func = resolver->find("main");
        if(main_func && main_func->kind() == ASTNodeKind::FunctionDecl) {
            if (options->verbose) {
                std::cout << "[lab] " << "making found 'main' function no_mangle" << std::endl;
            }
            main_func->as_function_unsafe()->set_no_mangle(true);
        }
    }

    resolver->module_scope_end(mod_index);
    resolver->stored_file_symbols.clear();
    resolver->current_mod_scope = prev_mod_scope;
    return 0;
}

int ASTProcessor::sym_res_module_seq(LabModule* module) {
    const auto mod_index = resolver->module_scope_start();

    // declare symbols for all files once in the module

    for(auto& file_ptr : module->direct_files) {

        auto& file = *file_ptr.result;

        sym_res_declare_and_link_file(file.unit.scope.body, file.file_id, file.abs_path);

        // report and clear diagnostics
        if (resolver->has_errors && !options->ignore_errors) {
            std::cerr << rang::fg::red << "couldn't perform job due to errors during symbol resolution" << rang::fg::reset << std::endl;
            return 1;
        }
        resolver->reset_errors();

        // clear everything allocated during symbol resolution of current file
        file_allocator.clear();

    }

    resolver->module_scope_end(mod_index);
    return 0;
}

void ASTProcessor::print_benchmarks(std::ostream& stream, const std::string_view& TAG, BenchmarkResults* bm_results) {
    stream << '[' << TAG << ']' << " completed " << bm_results->representation() << std::endl;
}

void ASTProcessor::print_benchmarks(std::ostream& stream, const std::string_view& TAG, const std::string_view& Name, BenchmarkResults* bm_results) {
    stream << '[' << TAG << ']' << ' ' << '\'' << Name << "' completed " << bm_results->representation() << std::endl;
}


// this imports a chemical file without any imports
ASTFileResult* chem_file_concur_importer_direct(
        int id,
        ASTProcessor* processor,
        ASTFileMetaData& fileData,
        bool use_job_allocator
) {
    // import the file
    processor->import_chemical_file(*fileData.result, fileData.file_id, fileData.abs_path, use_job_allocator);

    // print the file results
    processor->print_file_results(*fileData.result, chem::string_view(fileData.abs_path), processor->options->benchmark_files);

    // return the result
    return fileData.result;
}

#ifdef DEBUG
#define DEBUG_FUTURE false
#endif

bool ASTProcessor::import_chemical_files_direct(
        ctpl::thread_pool& pool,
        std::vector<ASTFileMetaData>& files
) {
    std::vector<std::future<ASTFileResult*>> futures;
    futures.reserve(files.size());

    // launch all files concurrently
    for(auto& fileData : files) {

        const auto file_id = fileData.file_id;
        const auto& abs_path = fileData.abs_path;

        // we must try to store chem::string_view into the fileData, from the beginning
        const auto ptr = new ASTFileResult(file_id, "", fileData.module);
        // copy the metadata into it
        *static_cast<ASTFileMetaData*>(ptr) = fileData;
        fileData.result = ptr;
        // cache uses std::unique_ptr to destruct it
        // we must store in cache, otherwise we'll have a memory leak
        cache.emplace(file_id, ptr);

#if defined(DEBUG_FUTURE) && DEBUG_FUTURE
        std::promise<bool> promise;
        promise.set_value(chem_file_concur_importer_direct(0, this, fileData, false));
        futures.emplace_back(promise.get_future());
#else
        futures.emplace_back(
                pool.push(chem_file_concur_importer_direct, this, fileData, false)
        );
#endif

    }

    // put each file sequentially into the out_files
    for(auto& wrap : futures) {
        const auto result = wrap.get();
        if(!result->continue_processing) {
            return false;
        }
    }

    return true;
}

struct TypeVerifyFileResult {
    bool has_errors = false;
    std::vector<Diag> diagnostics;
};

TypeVerifyFileResult type_verify_file_task(
    ASTProcessor* processor,
    ASTFileResult* file
) {
    TypeVerifyFileResult result;
    // make a local diagnoser
    ASTDiagnoser diagnoser(processor->loc_man);

    // run verification
    type_verify(diagnoser, processor->file_allocator, file->unit.scope.body.nodes);

    result.has_errors = diagnoser.has_errors;
    result.diagnostics = std::move(diagnoser.diagnostics);
    return result;
}

bool ASTProcessor::type_verify_module_parallel(ctpl::thread_pool& pool, LabModule* module) {
    std::vector<std::future<TypeVerifyFileResult>> futures;
    futures.reserve(module->direct_files.size());

    for(auto& fileData : module->direct_files) {
        // push task
        futures.emplace_back(pool.push([this, &fileData](int id){
            return type_verify_file_task(this, fileData.result);
        }));
    }

    bool success = true;
    int i = 0;
    for(auto& f : futures) {
        auto res = f.get();
        if(res.has_errors) {
            success = false;
        }
        if(!res.diagnostics.empty()) {
            std::lock_guard<std::mutex> guard(print_mutex);
            // print diagnostics
            Diagnoser::print_diagnostics(res.diagnostics, chem::string_view(module->direct_files[i].abs_path), "TypeCheck");
        }
        i++;
    }

    return success;
}

bool ASTProcessor::import_chemical_files_recursive(
        ctpl::thread_pool& pool,
        ConcurrentParsingState& state,
        std::vector<ASTFileMetaData>& files,
        bool use_job_allocator
) {

    if(files.empty()) {
        return true;
    }

    // if has imports, we import those files
    // launch all files concurrently
    for(auto& fileData : files) {

        const auto file_id = fileData.file_id;
        const auto& abs_path = fileData.abs_path;

        {
            std::lock_guard<std::mutex> guard(import_mutex);
            auto found = cache.find(file_id);
            if (found != cache.end()) {
                fileData.result = found->second.get();
                // inform the import statement about the file result
                if(fileData.stmt) {
                    fileData.stmt->setResult(fileData.result);
                }
                continue;
            }

            // we must try to store chem::string_view into the fileData, from the beginning
            const auto ptr = new ASTFileResult(file_id, "", fileData.module);
            fileData.result = ptr;
            // inform the import statement about the file result
            if(fileData.stmt) {
                fileData.stmt->setResult(ptr);
            }
            *static_cast<ASTFileMetaData*>(ptr) = fileData;
            cache.emplace(file_id, ptr);
        }

        // this is done inside the mutex lock, so to prevent race conditions when appending
        // to futures vector
        state.pushed_task();
        pool.push([this, &pool, &state, &fileData, use_job_allocator](int){
            const auto result = import_chemical_file_recursive(*fileData.result, pool, state, fileData, use_job_allocator);
            state.done_task();
            return result;
        });

    }

    return true;

}

bool ASTProcessor::figure_out_direct_imports(
        ASTFileMetaData& fileData,
        std::vector<ASTNode*>& fileNodes,
        std::vector<ASTFileMetaData>& outImports
) {
    for(auto node : fileNodes) {

        if(node->kind() == ASTNodeKind::ImportStmt) {

            auto stmt = node->as_import_stmt_unsafe();

            if(!stmt->isLocalFileImport()) {
                // skip native, local or remote module imports
                continue;
            }

            const auto& path = stmt->getSourcePath();

            // resolve the import path of this import statement
            auto replaceResult = path_handler.resolve_import_path(fileData.abs_path, path.view());

            if(replaceResult.error.empty()) {

                auto fileId = loc_man.encodeFile(replaceResult.replaced);

                auto module = fileData.module;

                // here since this path begins with '@'
                // we must determine which module it belongs to, we index modules based on 'scope_name:module_name' format
                // sometimes the scope is empty, so user can directly use -> import "@std/fs.ch"
                if(path[0] == '@') {

                    // get at directive, which contains module name
                    const auto atDirective = path_handler.get_atDirective(path.view());

                    // determine module
                    const auto found = mod_storage.find_module(atDirective.replaced);
                    if(found) {

                        module = &found->module_scope;

                    } else {

                        // this is an error, because we couldn't determine the module for this file
                        // even though it's path begins with a '@' which means it's external to module

                        // TODO: error out here because we couldn't find the module
                        // error_out(loc_man, stmt->encoded_location(), "couldn't determine module");
                        // return false;

                    }

                }

                outImports.emplace_back(fileId, module, std::move(replaceResult.replaced), stmt);

            } else {

                std::cerr << "[lab] " << rang::fg::red << "error: " << rang::fg::reset << "couldn't replace '@' in the path at ";
                std::cerr << loc_man.formatLocation(stmt->encoded_location()) << std::endl;
                return false;

            }

        } else {

            // import statements are supposed to be at the top
            break;

        }

    }

    return true;

}

bool ASTProcessor::import_mod_file_as_lab(
        ASTFileMetaData& meta,
        ASTFileResult& result,
        bool use_job_allocator,
        InputSource* inp_source
) {

    // get the file id
    auto& modFile = meta.abs_path;
    const auto modFileId = meta.file_id;

    // importing .mod file into data
    ModuleFileData data(meta);
    auto importModFileRes = import_chemical_mod_file(file_allocator, file_allocator, loc_man, data, modFileId, modFile, inp_source);
    if(!importModFileRes) {
        return false;
    }

    // lets use it to translate module file into a build.lab and import it
    std::ostringstream stream;
    convertToBuildLab(data, stream);
    const auto labOut = stream.view();
    InputSource labInpSource(labOut.data(), labOut.size());

    // import the file into result
    return import_chemical_file(result, modFileId, modFile, &labInpSource, use_job_allocator);

}

std::optional<FileInputSource> ASTProcessor::make_file_input_source(const char* abs_path, ASTFileResult& result) {
    FileInputSource inp_source(abs_path);
    const auto err = inp_source.error();
    if(err != nullptr) {
        result.continue_processing = false;
        result.read_error = err->format();
        std::cerr << rang::fg::red << "error: when reading file " << abs_path;
        if(!result.read_error.empty()) {
            std::cerr << " because " << result.read_error;
        }
        std::cerr << rang::fg::reset << std::endl;
        return std::nullopt;
    }
    return inp_source;
}

bool import_file_in_lab(
        ASTProcessor& proc,
        ASTFileMetaData& meta,
        ASTFileResult& result,
        bool use_job_allocator
) {
    auto& abs_path = meta.abs_path;
    const auto fileId = meta.file_id;
    // import the file if it has no error
    FileInputSource inp_source(abs_path.data());
    const auto err = inp_source.error();
    if(err == nullptr) {
        // import the file into result (lex and parse)
        return proc.import_chemical_file(result, fileId, abs_path, &inp_source, use_job_allocator);
    }

    // since an error occurred, if this is an import for build.lab file
    // and instead of a build.lab file user has a chemical.mod file, we translate it
    auto prev_msg = err->format();
    if(abs_path.ends_with(".lab")) {
        auto modFile = resolve_sibling(abs_path, "chemical.mod");
        const auto modFileErr = inp_source.open(modFile.data());
        if(!modFileErr) {

            // lets get a new file id for module file
            auto modFileId = proc.loc_man.encodeFile(modFile);

            // we also change the absolute path of the file to the new chemical.mod file
            meta.file_id = modFileId;
            meta.abs_path = modFile;

            return proc.import_mod_file_as_lab(meta, result, use_job_allocator, &inp_source);

        }
    }

    // since couldn't import both .lab / .mod we return the original error
    result.continue_processing = false;
    result.read_error = prev_msg;
    std::cerr << rang::fg::red << "error: when reading file '" << abs_path;
    if(!result.read_error.empty()) {
        std::cerr << "' because " << result.read_error;
    }
    std::cerr << rang::fg::reset << std::endl;
    return false;

}

bool ASTProcessor::import_chemical_file_recursive(
        ASTFileResult& result,
        ctpl::thread_pool& pool,
        ConcurrentParsingState& state,
        ASTFileMetaData& parentFileData,
        bool use_job_allocator
) {

    // import the file into result (lex and parse)
    const auto success = import_file_in_lab(*this, parentFileData, result, use_job_allocator);
    if(!success) {
        return false;
    }

    // figure out files imported by this file
    const auto success2 = figure_out_direct_imports(parentFileData, result.unit.scope.body.nodes, result.imports);
    if(!success2) {
        return false;
    }

    // handle the imports of this file
    return import_chemical_files_recursive(pool, state, result.imports, use_job_allocator);

}

// this imports a chemical file without any imports
std::pair<ASTFileResult*, std::vector<Token>> chem_file_concur_importer_direct_toks(
        int id,
        ASTProcessor* processor,
        ASTFileMetaData& fileData,
        bool use_job_allocator,
        bool keep_comments
) {

    // out tokens
    std::vector<Token> tokens;

    // import the file
    processor->import_chemical_file_with_tokens(*fileData.result, fileData.file_id, fileData.abs_path, use_job_allocator, &tokens, keep_comments);

    // print the file results
    processor->print_file_results(*fileData.result, chem::string_view(fileData.abs_path), processor->options->benchmark_files);

    // return the result
    return { fileData.result, std::move(tokens) };
}

bool ASTProcessor::import_chemical_files_direct_with_tokens(
        ctpl::thread_pool& pool,
        std::vector<ASTFileMetaData>& files,
        std::unordered_map<unsigned int, std::vector<Token>>& token_map,
        bool keep_comments
) {
    std::vector<std::future<std::pair<ASTFileResult*, std::vector<Token>>>> futures;
    futures.reserve(files.size());

    // launch all files concurrently
    for(auto& fileData : files) {

        const auto file_id = fileData.file_id;
        const auto& abs_path = fileData.abs_path;

        // we must try to store chem::string_view into the fileData, from the beginning
        const auto ptr = new ASTFileResult(file_id, "", fileData.module);
        // copy the metadata into it
        *static_cast<ASTFileMetaData*>(ptr) = fileData;
        fileData.result = ptr;
        // cache uses std::unique_ptr to destruct it
        // we must store in cache, otherwise we'll have a memory leak
        cache.emplace(file_id, ptr);

#if defined(DEBUG_FUTURE) && DEBUG_FUTURE
        std::promise<bool> promise;
        promise.set_value(chem_file_concur_importer_direct_toks(0, this, fileData, false, keep_comments));
        futures.emplace_back(promise.get_future());
#else
        futures.emplace_back(
                pool.push(chem_file_concur_importer_direct_toks, this, fileData, false, keep_comments)
        );
#endif

    }

    // put each file sequentially into the out_files
    for(auto& wrap : futures) {
        const auto result = wrap.get();
        if(!result.first->continue_processing) {
            return false;
        }
        token_map[result.first->file_id] = std::move(result.second);
    }

    return true;
}

void ASTProcessor::print_file_results(ASTFileResult& result, const chem::string_view& abs_path, bool benchmark) {
    std::lock_guard print_lock(print_mutex);
    print_results(result, abs_path, benchmark);
    // after printing we dispose the logs
    auto moved1 = std::move(result.lex_diagnostics);
    auto moved2 = std::move(result.parse_diagnostics);
}

bool ASTProcessor::import_chemical_file(
        ASTFileResult& result,
        unsigned int fileId,
        const std::string_view& abs_path,
        InputSource* inp_source,
        bool use_job_allocator
) {

    result.abs_path = abs_path;
    result.file_id = fileId;
    result.private_symbol_range = { 0, 0 };
    result.continue_processing = true;
    result.diCompileUnit = nullptr;

    auto& unit = result.unit;

    Lexer lexer(std::string(abs_path), *inp_source, &binder, file_allocator);
    std::vector<Token> tokens;

    const auto benchmark = options->benchmark_files;

    // actual lexing
    if(benchmark) {
        result.lex_benchmark.benchmark_begin();
        lexer.getTokens(tokens);
        result.lex_benchmark.benchmark_end();
    } else {
        lexer.getTokens(tokens);
    }

    const auto total_toks = tokens.size();
    if(total_toks == 0) {
        // empty file
        return true;
    } else if(tokens[total_toks - 1].type == TokenType::Unexpected) {
        auto& last = tokens[total_toks - 1];
        lexer.diagnoser.diagnostic(last.value, chem::string_view(result.unit.scope.getAbsPath()), last.position, last.position, DiagSeverity::Error);
    }

    // move lexer diagnostics
    if(!lexer.diagnoser.diagnostics.empty()) {
        result.lex_diagnostics = std::move(lexer.diagnoser.diagnostics);
    }

    // do not continue, if error occurs during lexing
    if(lexer.diagnoser.has_errors) {
        result.continue_processing = false;
        return false;
    }

    // parse the file
    Parser parser(
            fileId,
            abs_path,
            tokens.data(),
            resolver->comptime_scope.loc_man,
            controller,
            job_allocator,
            use_job_allocator ? job_allocator : mod_allocator,
            type_builder,
            resolver->is64Bit,
            &binder
    );

    // setting file scope as parent of all nodes parsed
    parser.parent_node = &result.unit.scope;

    // actual parsing
    if(benchmark) {
        result.parse_benchmark.benchmark_begin();
        parser.parse(unit.scope.body.nodes);
        result.parse_benchmark.benchmark_end();
    } else {
        parser.parse(unit.scope.body.nodes);
    }

    // move parser diagnostics
    if(!parser.diagnostics.empty()) {
        result.parse_diagnostics = std::move(parser.diagnostics);
    }

    // continue
    if(parser.has_errors) {
        result.continue_processing = false;
        return false;
    } else {
        return true;
    }
}

bool ASTProcessor::import_chemical_file_with_tokens(
        ASTFileResult& result,
        unsigned int fileId,
        const std::string_view& abs_path,
        InputSource* inp_source,
        bool use_job_allocator,
        std::vector<Token>* out_tokens,
        bool keep_comments
) {

    result.abs_path = abs_path;
    result.file_id = fileId;
    result.private_symbol_range = { 0, 0 };
    result.continue_processing = true;
    result.diCompileUnit = nullptr;

    auto& unit = result.unit;

    Lexer lexer(std::string(abs_path), *inp_source, &binder, file_allocator);
    lexer.keep_comments = keep_comments;
    std::vector<Token> tokens;

    const auto benchmark = options->benchmark_files;

    // actual lexing
    if(benchmark) {
        result.lex_benchmark.benchmark_begin();
        lexer.getTokens(tokens);
        result.lex_benchmark.benchmark_end();
    } else {
        lexer.getTokens(tokens);
    }

    const auto total_toks = tokens.size();
    if(total_toks == 0) {
        // empty file
        return true;
    } else if(tokens[total_toks - 1].type == TokenType::Unexpected) {
        auto& last = tokens[total_toks - 1];
        lexer.diagnoser.diagnostic(last.value, chem::string_view(result.unit.scope.getAbsPath()), last.position, last.position, DiagSeverity::Error);
    }

    // copy tokens if requested
    if(out_tokens != nullptr) {
        *out_tokens = tokens;
    }

    // move lexer diagnostics
    if(!lexer.diagnoser.diagnostics.empty()) {
        result.lex_diagnostics = std::move(lexer.diagnoser.diagnostics);
    }

    // do not continue, if error occurs during lexing
    if(lexer.diagnoser.has_errors) {
        result.continue_processing = false;
        return false;
    }

    // we should remove the comment tokens before giving them to the parser
    // parser does not expect comments or whitespaces
    std::vector<Token> clean_tokens;
    Token* tokens_to_parse = tokens.data();

    if(keep_comments) {
        clean_tokens.reserve(tokens.size());
        for(auto& t : tokens) {
            if(t.type != TokenType::SingleLineComment && t.type != TokenType::MultiLineComment && t.type != TokenType::Whitespace) {
                clean_tokens.push_back(t);
            }
        }
        tokens_to_parse = clean_tokens.data();
    }

    // parse the file
    Parser parser(
            fileId,
            abs_path,
            tokens_to_parse,
            resolver->comptime_scope.loc_man,
            controller,
            job_allocator,
            use_job_allocator ? job_allocator : mod_allocator,
            type_builder,
            resolver->is64Bit,
            &binder
    );

    // setting file scope as parent of all nodes parsed
    parser.parent_node = &result.unit.scope;

    // actual parsing
    if(benchmark) {
        result.parse_benchmark.benchmark_begin();
        parser.parse(unit.scope.body.nodes);
        result.parse_benchmark.benchmark_end();
    } else {
        parser.parse(unit.scope.body.nodes);
    }

    // move parser diagnostics
    if(!parser.diagnostics.empty()) {
        result.parse_diagnostics = std::move(parser.diagnostics);
    }

    // continue
    if(parser.has_errors) {
        result.continue_processing = false;
        return false;
    } else {
        return true;
    }

}

bool ASTProcessor::import_chemical_mod_file(
        ASTAllocator& fileAllocator,
        ASTAllocator& modAllocator,
        LocationManager& loc_man,
        ModuleFileData& data,
        unsigned int fileId,
        const std::string_view& abs_path,
        InputSource* inp_source
) {

    Lexer lexer(std::string(abs_path), *inp_source, nullptr, fileAllocator);
    std::vector<Token> tokens;
    lexer.getTokens(tokens);

    // parse the file
    BasicParser parser(
        loc_man,
        fileId,
        tokens.data()
    );

    // put the lexing diagnostic into the parser diagnostic for now
    if(!tokens.empty()) {
        auto& last_token = tokens.back();
        if (last_token.type == TokenType::Unexpected) {
            parser.diagnostics.emplace_back(Diagnoser::make_diag("[DEBUG_TRAD_LEXER] unexpected token is at last", chem::string_view(abs_path), last_token.position, last_token.position, DiagSeverity::Warning));
        }
    }

    // setting file scope as parent of all nodes parsed
    parser.parent_node = &data.scope;

    parser.parseModuleFile(modAllocator, data);

    data.diagnostics = std::move(parser.diagnostics);

    return !parser.has_errors;

}

bool ASTProcessor::import_chemical_mod_file(
        ASTAllocator& fileAllocator,
        ASTAllocator& modAllocator,
        LocationManager& loc_man,
        ModuleFileData& data,
        unsigned int fileId,
        const std::string_view& abs_path
) {
    FileInputSource inp_source(abs_path.data());
    const auto err = inp_source.error();
    if(err != nullptr) {
        data.read_error = err->format();
        std::cerr << rang::fg::red << "error: when reading file " << abs_path;
        if(!data.read_error.empty()) {
            std::cerr << " because " << data.read_error;
        }
        std::cerr << rang::fg::reset << std::endl;
        return false;
    }
    return import_chemical_mod_file(fileAllocator, modAllocator, loc_man, data, fileId, abs_path, &inp_source);
}

void ASTProcessor::declare_before_translation(
        ToCAstVisitor& visitor,
        std::vector<ASTNode*>& nodes,
        const std::string& abs_path
) {
    // translating the nodes
    BenchmarkResults bm_results;
    if(options->benchmark_files) {
        bm_results.benchmark_begin();
    }
    visitor.declare_before_translation(nodes);
    if(options->benchmark_files) {
        bm_results.benchmark_end();
        print_benchmarks(std::cout, "2cTranslation:declare", &bm_results);
    }
    if(!visitor.diagnostics.empty()) {
        visitor.print_diagnostics(chem::string_view(abs_path), "2cTranslation");
        std::cout << std::endl;
    }
    visitor.reset_errors();
}

void ASTProcessor::translate_after_declaration(
        ToCAstVisitor& visitor,
        std::vector<ASTNode*>& nodes,
        const std::string& abs_path
) {
    // translating the nodes
    BenchmarkResults bm_results;
    if(options->benchmark_files) {
        bm_results.benchmark_begin();
    }
    visitor.translate_after_declaration(nodes);
    if(options->benchmark_files) {
        bm_results.benchmark_end();
        print_benchmarks(std::cout, "2cTranslation:translate", abs_path, &bm_results);
    }
    if(!visitor.diagnostics.empty()) {
        visitor.print_diagnostics(chem::string_view(abs_path), "2cTranslation");
        std::cout << std::endl;
    }
    visitor.reset_errors();
}

void ASTProcessor::translate_to_c(
        ToCAstVisitor& visitor,
        std::vector<ASTNode*>& nodes,
        const std::string& abs_path
) {
    // translating the nodes
    BenchmarkResults bm_results;
    if(options->benchmark_files) {
        bm_results.benchmark_begin();
    }
    visitor.declare_and_translate(nodes);
    if(options->benchmark_files) {
        bm_results.benchmark_end();
        print_benchmarks(std::cout, "2cTranslation", &bm_results);
    }
    if(!visitor.diagnostics.empty()) {
        visitor.print_diagnostics(chem::string_view(abs_path), "2cTranslation");
        std::cout << std::endl;
    }
    visitor.reset_errors();
}

void ASTProcessor::external_declare_in_c(
        ToCAstVisitor& visitor,
        Scope& import_res,
        const std::string& abs_path
) {
    visitor.external_declare(import_res.nodes);
    if(!visitor.diagnostics.empty()) {
        visitor.print_diagnostics(chem::string_view(abs_path), "2cTranslation");
        std::cout << std::endl;
    }
    visitor.reset_errors();
}

void ASTProcessor::external_implement_in_c(
        ToCAstVisitor& visitor,
        Scope& import_res,
        const std::string& abs_path
) {
    visitor.external_implement(import_res.nodes);
    if(!visitor.diagnostics.empty()) {
        visitor.print_diagnostics(chem::string_view(abs_path), "2cTranslation");
        std::cout << std::endl;
    }
    visitor.reset_errors();
}

/**
 * these two functions have following purpose
 * 1 - we need to process only direct dependencies, so 'outMods' only contains modules that are given to us in 'inMods'
 * 2 - this sorts the modules in the order of independence, so modules that are least dependent are processed first
 *   - this way if module a depends on module b, however both are present in inMods (in any order), we'll process b first since a needs it
 * 3 - a module will only appear once, however inMods could contain the module as many times as it wants
 */
void shallow_dedupe_sorted(std::vector<LabModule*>& outMods, std::unordered_map<LabModule*, bool>& modsMap, LabModule* mod) {
    for(auto& dep : mod->dependencies) {
        shallow_dedupe_sorted(outMods, modsMap, dep.module);
    }
    auto found = modsMap.find(mod);
    if(found != modsMap.end() && found->second) {
        outMods.emplace_back(mod);
        found->second = false;
    }
}
void shallow_dedupe_sorted(std::vector<LabModule*>& outMods, std::vector<ModuleDependency>& deps) {
    std::unordered_map<LabModule*, bool> direct_mods(deps.size());
    // save these modules in direct_mods
    for(auto& mod : deps) {
        direct_mods[mod.module] = true;
    }
    for(auto& dep : deps) {
        shallow_dedupe_sorted(outMods, direct_mods, dep.module);
    }
}

void forward_declare_in_c(ToCAstVisitor& c_visitor, ASTProcessor* proc, LabModule* mod, const char* name) {
    for(auto& file_ptr : mod->direct_files) {
        auto& result = *file_ptr.result;
        auto& file = result;
        ASTUnit& unit = file.unit;
        // print the benchmark or verbose output received from processing
        if((proc->options->benchmark_files || proc->options->verbose) && !ASTProcessor::empty_diags(result)) {
            std::cout << rang::style::bold << rang::fg::magenta << '[' << name << "] " << file.abs_path << rang::fg::reset << rang::style::reset << '\n';
        }

#ifdef DEBUG
        auto str = (name + std::string(" ") + file.abs_path);
        c_visitor.debug_comment(chem::string_view(str));
#endif

        c_visitor.fwd_declare(unit.scope.body.nodes);
    }
}

void declare_type_aliases_in_c(ToCAstVisitor& c_visitor, ASTProcessor* proc, LabModule* mod, const char* name) {
    for(auto& file_ptr : mod->direct_files) {
        auto& result = *file_ptr.result;
        auto& file = result;
        ASTUnit& unit = file.unit;
        // print the benchmark or verbose output received from processing
        if((proc->options->benchmark || proc->options->verbose) && !ASTProcessor::empty_diags(result)) {
            std::cout << rang::style::bold << rang::fg::magenta << '[' << name << "] " << file.abs_path << rang::fg::reset << rang::style::reset << '\n';
        }

#ifdef DEBUG
        auto str = (name + std::string(" ") + file.abs_path);
        c_visitor.debug_comment(chem::string_view(str));
#endif

        c_visitor.declare_type_aliases(unit.scope.body.nodes);
    }
}

int ASTProcessor::translate_module(
    ToCAstVisitor& c_visitor,
    LabModule* module
) {

    // let's create a flat vector of direct dependencies, that we want to process
    std::vector<LabModule*> dependencies;
    shallow_dedupe_sorted(dependencies, module->dependencies);

    // forward declare dependencies & dependencies of dependencies & current module
    forward_declare_in_c(c_visitor, this, module, "FwdDeclare");

    // declare type aliases
    declare_type_aliases_in_c(c_visitor, this, module, "TypeAliasDeclare");

    // we will forward declare the direct dependencies of this module
    // only the newly introduced generics
    for(const auto dep : dependencies) {
        for(auto& file : dep->direct_files) {
            if(file.result != nullptr) {

                auto& body = file.result->unit.scope.body;

#ifdef DEBUG
                c_visitor.debug_comment(chem::string_view(("ExtFwdDeclare " + file.abs_path)));
#endif
                c_visitor.ext_fwd_declare(body.nodes);
            } else {
                CHEM_THROW_RUNTIME("result is null");
            }
        }
    }

    // we will declare the direct dependencies of this module
    for(const auto dep : dependencies) {
        for(auto& file : dep->direct_files) {
            if(file.result != nullptr) {

                auto& body = file.result->unit.scope.body;

#ifdef DEBUG
                c_visitor.debug_comment(chem::string_view(("ExtDeclare " + file.abs_path)));
#endif

                external_declare_in_c(c_visitor, body, file.abs_path);
            } else {
                CHEM_THROW_RUNTIME("result is null");
            }
        }
    }

    // The second loop deals with declaring files that are present in this module
    // declaring means (only prototypes, no function bodies, struct prototypes...)
    for(auto& file_ptr : module->direct_files) {

        auto& file = *file_ptr.result;
        auto& unit = file.unit;

        // print the benchmark or verbose output received from processing
        if((options->benchmark_files || options->verbose) && !empty_diags(file)) {
            std::cout << rang::style::bold << rang::fg::magenta << "[Declare] " << file.abs_path << rang::fg::reset << rang::style::reset << '\n';
        }

#ifdef DEBUG
        c_visitor.debug_comment(chem::string_view(("Declare " + file.abs_path)));
#endif

        declare_before_translation(c_visitor, unit.scope.body.nodes, file.abs_path);

    }

    // we will implement the direct dependencies of this module
    for(const auto dep : dependencies) {
        for(auto& file : dep->direct_files) {
            auto& body = file.result->unit.scope.body;

#ifdef DEBUG
            c_visitor.debug_comment(chem::string_view(("ExtImplement " + file.abs_path)));
#endif

            external_implement_in_c(c_visitor, body, file.abs_path);
        }
    }

    // The fourth loop deals with generating function bodies present in the current module
    for(auto& file_ptr : module->direct_files) {

        auto& file = *file_ptr.result;
        auto& result = file;

        ASTUnit& unit = file.unit;

#ifdef DEBUG
        c_visitor.debug_comment(chem::string_view(("Translate " + file.abs_path)));
#endif

        // translating to c
        translate_after_declaration(c_visitor, unit.scope.body.nodes, file.abs_path);

        // clear everything we allocated using file allocator to make it re-usable
        file_allocator.clear();

    }

    // resetting c visitor to use with another module
    c_visitor.reset();

    // return for success
    return 0;

}