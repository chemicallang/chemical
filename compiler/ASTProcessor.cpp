// Copyright (c) Chemical Language Foundation 2025.

#include "ASTProcessor.h"

#include <memory>
#include "parser/Parser.h"
#include "compiler/SymbolResolver.h"
#include "preprocess/2c/2cASTVisitor.h"
#include "utils/Benchmark.h"
#include <sstream>
#include "utils/PathUtils.h"
#include "compiler/lab/LabBuildCompiler.h"
#include "std/chem_string.h"
#include "rang.hpp"
#include "preprocess/RepresentationVisitor.h"
#include "ast/structures/FunctionDeclaration.h"
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
    return result.lex_diagnostics.empty() && result.parse_diagnostics.empty() && !result.lex_benchmark && !result.parse_benchmark;
}

void ASTProcessor::print_results(ASTFileResult& result, const chem::string_view& abs_path, bool benchmark) {
    if(!empty_diags(result)) {
        std::cout << rang::style::bold << rang::fg::magenta << "[Parsed] " << abs_path << rang::fg::reset << rang::style::reset << '\n';
        Diagnoser::print_diagnostics(result.lex_diagnostics, abs_path, "Lexer");
        Diagnoser::print_diagnostics(result.parse_diagnostics, abs_path, "Parser");
        if (benchmark) {
            if (result.lex_benchmark) {
                ASTProcessor::print_benchmarks(std::cout, "Lexer", result.lex_benchmark.get());
            }
            if (result.parse_benchmark) {
                ASTProcessor::print_benchmarks(std::cout, "Parser", result.parse_benchmark.get());
            }
        }
        std::cout << std::flush;
        // we clear these diagnostics after printing, so next call to print_results doesn't print them
        result.lex_diagnostics.clear();
        result.parse_diagnostics.clear();
        result.lex_benchmark = nullptr;
        result.parse_benchmark = nullptr;
    }
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
    path_handler.module_src_dir_path = "";
    switch(module->type) {
        case LabModuleType::Files: {
            for (auto& str: module->paths) {
                auto abs_path = canonical_path(str.to_view());
                if (abs_path.empty()) {
                    std::cerr << rang::fg::red << "error: " << rang::fg::reset << "couldn't determine canonical path for file '" << str.data() << "' in module '" << module->name << '\'' << std::endl;
                    continue;
                }
                auto fileId = loc_man.encodeFile(abs_path);
                // all these files belong to the given module, so it's scope will be used
                files.emplace_back(fileId, &module->module_scope, abs_path, abs_path, "");
            }
            return;
        }
        case LabModuleType::ObjFile:
        case LabModuleType::CFile:
        case LabModuleType::CPPFile:
            return;
        case LabModuleType::Directory:
            if(module->paths.size() == 1) {
                // TODO: this should be removed !
                path_handler.module_src_dir_path = module->paths[0].to_view();
            }
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
                        files.emplace_back(fileId, &module->module_scope, abs_path, abs_path, "");
                    }
                } else if(dir_path.ends_with(".ch")) {
                    auto dir_path_view = dir_path.to_view();
                    auto abs_path = canonical_path(dir_path_view);
                    if(abs_path.empty()) {
                        std::cerr << rang::fg::red << "error: " << rang::fg::reset << "couldn't determine canonical path for file '" << dir_path_view << "' in module '" << *module << '\'' << std::endl;
                        continue;
                    } else {
                        auto fileId = loc_man.encodeFile(abs_path);
                        files.emplace_back(fileId, &module->module_scope, std::string(dir_path.to_view()), abs_path, "");
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
    // TODO: module's src dir path should not be visible to path handler
    if(module->type == LabModuleType::Directory && module->paths.size() == 1) {
        path_handler.module_src_dir_path = module->paths[0].to_view();
    } else {
        path_handler.module_src_dir_path = "";
    }
    return import_chemical_files_direct(pool, files);
}

SymbolRange ASTProcessor::sym_res_tld_declare_file(Scope& scope, unsigned int fileId, const std::string& abs_path) {
    // doing stuff
    auto prev_has_errors = resolver->has_errors;
    std::unique_ptr<BenchmarkResults> bm_results;
    if(options->benchmark) {
        bm_results = std::make_unique<BenchmarkResults>();
        bm_results->benchmark_begin();
    }
    const auto range = resolver->tld_declare_file(scope, fileId, abs_path);
    if(options->benchmark) {
        bm_results->benchmark_end();
        print_benchmarks(std::cout, "SymRes:declare", abs_path, bm_results.get());
    }
    if(!resolver->diagnostics.empty()) {
        resolver->print_diagnostics(chem::string_view(abs_path), "SymRes:declare");
    }
    return range;
}

void ASTProcessor::sym_res_link_sig_file(Scope& scope, unsigned int fileId, const std::string& abs_path, const SymbolRange& range) {
    // doing stuff
    auto prev_has_errors = resolver->has_errors;
    std::unique_ptr<BenchmarkResults> bm_results;
    if(options->benchmark) {
        bm_results = std::make_unique<BenchmarkResults>();
        bm_results->benchmark_begin();
    }
    resolver->link_signature_file(scope, fileId, range);
    if(options->benchmark) {
        bm_results->benchmark_end();
        print_benchmarks(std::cout, "SymRes:link_sig", abs_path, bm_results.get());
    }
    if(!resolver->diagnostics.empty()) {
        resolver->print_diagnostics(chem::string_view(abs_path), "SymRes:link_sig");
    }
}

void ASTProcessor::sym_res_link_file(Scope& scope, unsigned int fileId, const std::string& abs_path, const SymbolRange& range) {
    // doing stuff
    auto prev_has_errors = resolver->has_errors;
    std::unique_ptr<BenchmarkResults> bm_results;
    if(options->benchmark) {
        bm_results = std::make_unique<BenchmarkResults>();
        bm_results->benchmark_begin();
    }
    resolver->link_file(scope, fileId, range);
    if(options->benchmark) {
        bm_results->benchmark_end();
        print_benchmarks(std::cout, "SymRes:link", abs_path, bm_results.get());
    }
    if(!resolver->diagnostics.empty()) {
        resolver->print_diagnostics(chem::string_view(abs_path), "SymRes:link");
    }
}

void ASTProcessor::sym_res_declare_and_link_file(Scope& scope, unsigned int fileId, const std::string& abs_path) {
    // doing stuff
    auto prev_has_errors = resolver->has_errors;
    std::unique_ptr<BenchmarkResults> bm_results;
    if(options->benchmark) {
        bm_results = std::make_unique<BenchmarkResults>();
        bm_results->benchmark_begin();
    }
    resolver->declare_and_link_file(scope, fileId, abs_path);
    if(options->benchmark) {
        bm_results->benchmark_end();
        print_benchmarks(std::cout, "SymRes:link_seq", abs_path, bm_results.get());
    }
    if(!resolver->diagnostics.empty()) {
        resolver->print_diagnostics(chem::string_view(abs_path), "SymRes:link_seq");
    }
}

int ASTProcessor::sym_res_module(LabModule* module) {

    const auto mod_index = resolver->module_scope_start();

    // declare symbols of directly imported modules

    SymbolResolverDeclarer declarer(*resolver);
    for(const auto dep : module->dependencies) {
        for(auto& file_ptr : dep->direct_files) {
            auto& file = *file_ptr.result;
            for(const auto node : file.unit.scope.body.nodes) {
                declare_node(declarer, node, AccessSpecifier::Public);
            }
        }
    }

    // declare symbols for all files once in the module

    for(auto& file_ptr : module->direct_files) {

        auto& file = *file_ptr.result;

        file.private_symbol_range = sym_res_tld_declare_file(file.unit.scope.body, file.file_id, file.abs_path);

        // report and clear diagnostics
        if (resolver->has_errors && !options->ignore_errors) {
            std::cerr << rang::fg::red << "couldn't perform job due to errors during symbol resolution" << rang::fg::reset << std::endl;
            return 1;
        }
        resolver->reset_errors();

    }

    // link the signature of the files
    for(auto& file_ptr : module->direct_files) {

        auto& file = *file_ptr.result;

        sym_res_link_sig_file(file.unit.scope.body, file.file_id, file.abs_path, file.private_symbol_range);
        // report and clear diagnostics
        if (resolver->has_errors && !options->ignore_errors) {
            std::cerr << rang::fg::red << "couldn't perform job due to errors during symbol resolution" << rang::fg::reset << std::endl;
            return 1;
        }
        resolver->reset_errors();

    }

    // sequentially symbol resolve all the files in the module
    for(auto& file_ptr : module->direct_files) {

        auto& file = *file_ptr.result;

        sym_res_link_file(file.unit.scope.body, file.file_id, file.abs_path, file.private_symbol_range);
        if (resolver->has_errors && !options->ignore_errors) {
            std::cerr << rang::fg::red << "couldn't perform job due to errors during symbol resolution" << rang::fg::reset << std::endl;
            return 1;
        }
        resolver->reset_errors();

        // clear everything allocated during symbol resolution of current file
        file_allocator.clear();

    }

    // we need to search for main function in each module and make it no_mangle
    // so it won't be mangled (module scope and name gets added, which can cause no entry point error)
    const auto main_func = resolver->find("main");
    if(main_func && main_func->kind() == ASTNodeKind::FunctionDecl) {
        if(options->verbose) {
            std::cout << "[lab] " << "making found 'main' function no_mangle" << std::endl;
        }
        main_func->as_function_unsafe()->set_no_mangle(true);
    }

    resolver->module_scope_end(mod_index);
    resolver->stored_file_symbols.clear();
    return 0;
}

int ASTProcessor::sym_res_module_seq(LabModule* module) {
    const auto mod_index = resolver->module_scope_start();

    // declare symbols of directly imported modules

    SymbolResolverDeclarer declarer(*resolver);
    for(const auto dep : module->dependencies) {
        for(auto& file_ptr : dep->direct_files) {
            auto& file = *file_ptr.result;
            for(const auto node : file.unit.scope.body.nodes) {
                declare_node(declarer, node, AccessSpecifier::Public);
            }
        }
    }

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
    const auto mil = bm_results->millis();
    bool reset = false;
    if(mil > 1) {
        stream << (mil > 3 ? rang::fg::red : rang::fg::yellow);
        reset = true;
    }
    stream << '[' << TAG << ']' << " Completed " << bm_results->representation() << std::endl;
    if(reset) {
        stream << rang::fg::reset;
    }
}

void ASTProcessor::print_benchmarks(std::ostream& stream, const std::string_view& TAG, const std::string_view& ABS_PATH, BenchmarkResults* bm_results) {
    const auto mil = bm_results->millis();
    bool reset = false;
    if(mil > 1) {
        stream << (mil > 3 ? rang::fg::red : rang::fg::yellow);
        reset = true;
    }
    stream << '[' << TAG << ']' << ' ' << ABS_PATH << " Completed " << bm_results->representation() << std::endl;
    if(reset) {
        stream << rang::fg::reset;
    }
}


// this imports a chemical file without any imports
ASTFileResult* chem_file_concur_importer_direct(
        int id,
        ASTProcessor* processor,
        ASTFileResult* out_file,
        ASTFileMetaData& fileData,
        bool use_job_allocator
) {
    processor->import_file(*out_file, fileData.file_id, fileData.abs_path, use_job_allocator);
    return out_file;
}

// this imports a chemical file + imports
// handles relative import statements inside the file
ASTFileResult* chem_file_concur_importer_recursive(
        int id,
        ASTProcessor* processor,
        ctpl::thread_pool& pool,
        ASTFileResult* out_file,
        ASTFileMetaData& fileData,
        bool use_job_allocator
) {
    processor->import_chemical_file_recursive(*out_file, pool, fileData, use_job_allocator);
    return out_file;
}

struct future_ptr_union {
    ASTFileResult* result = nullptr;
    std::future<ASTFileResult*> future;
};

#ifdef DEBUG
#define DEBUG_FUTURE true
#endif

bool ASTProcessor::import_chemical_files_direct(
        ctpl::thread_pool& pool,
        std::vector<ASTFileMetaData>& files
) {
    std::vector<future_ptr_union> futures;

    // pointer variable to be used inside the for loop
    ASTFileResult* out_file;

    // launch all files concurrently
    for(auto& fileData : files) {

        const auto file_id = fileData.file_id;
        const auto& abs_path = fileData.abs_path;

        {
            std::lock_guard<std::mutex> guard(import_mutex);
            auto found = cache.find(abs_path);
            if (found != cache.end()) {
//                 std::cout << "not launching file : " << fileData.abs_path << std::endl;
                fileData.result = found->second.get();
                futures.emplace_back(found->second.get());
                continue;
            }

//            std::cout << "launching file : " << fileData.abs_path << std::endl;
            // we must try to store chem::string_view into the fileData, from the beginning
            const auto ptr = new ASTFileResult(file_id, "", fileData.module);
            fileData.result = ptr;
            cache.emplace(abs_path, ptr);
            out_file = ptr;
        }

        // copy the metadata into it
        *static_cast<ASTFileMetaData*>(out_file) = fileData;

#if defined(DEBUG_FUTURE) && DEBUG_FUTURE
        futures.emplace_back(chem_file_concur_importer_direct(0, this, out_file, fileData, false));
#else
        futures.emplace_back(
                nullptr,
                pool.push(chem_file_concur_importer_direct, this, out_file, fileData, false)
        );
#endif

    }

    // put each file sequentially into the out_files
    for(auto& wrap : futures) {
        const auto result = wrap.result ? wrap.result : wrap.future.get();
        if(!result->continue_processing) {
            return false;
        }
    }

    return true;
}

bool ASTProcessor::import_chemical_files_recursive(
        ctpl::thread_pool& pool,
        std::vector<ASTFileResult*>& out_files,
        std::vector<ASTFileMetaData>& files,
        bool use_job_allocator
) {

    std::vector<future_ptr_union> futures;

    // pointer variable to be used inside the for loop
    ASTFileResult* out_file;

    // launch all files concurrently
    for(auto& fileData : files) {

        const auto file_id = fileData.file_id;
        const auto& abs_path = fileData.abs_path;

        {
            std::lock_guard<std::mutex> guard(import_mutex);
            auto found = cache.find(abs_path);
            if (found != cache.end()) {
//                 std::cout << "not launching file : " << fileData.abs_path << std::endl;
                fileData.result = found->second.get();
                futures.emplace_back(found->second.get());
                continue;
            }

//            std::cout << "launching file : " << fileData.abs_path << std::endl;
            // we must try to store chem::string_view into the fileData, from the beginning
            const auto ptr = new ASTFileResult(file_id, "", fileData.module);
            fileData.result = ptr;
            cache.emplace(abs_path, ptr);
            out_file = ptr;
        }

        // copy the metadata into it
        *static_cast<ASTFileMetaData*>(out_file) = fileData;

#if defined(DEBUG_FUTURE) && DEBUG_FUTURE
        futures.emplace_back(chem_file_concur_importer_recursive(0, this, pool, out_file, fileData, use_job_allocator));
#else
        futures.emplace_back(
                nullptr,
                pool.push(chem_file_concur_importer_recursive, this, std::ref(pool), out_file, fileData, use_job_allocator)
        );
#endif

    }

    // put each file sequentially into the out_files
    for(auto& wrap : futures) {
        if(wrap.result) {
            out_files.emplace_back(wrap.result);
        } else {
            // ensure job completion
            out_files.emplace_back(wrap.future.get());
        }
    }

    for(const auto file : out_files) {
        if(!file->continue_processing) {
            return false;
        }
    }

    return true;

}

void ASTProcessor::figure_out_direct_imports(
        ASTFileMetaData& fileData,
        std::vector<ASTNode*>& fileNodes,
        std::vector<ASTFileMetaData>& outImports
) {
    for(auto node : fileNodes) {

        if(node->kind() == ASTNodeKind::ImportStmt) {

            auto stmt = node->as_import_stmt_unsafe();

            if(stmt->filePath.empty()) {
                // this must be 'import std' in build.lab
                // file path is only in double quotes import "file.ch"
                continue;
            }

            // resolve the import path of this import statement
            auto replaceResult = path_handler.resolve_import_path(fileData.abs_path, stmt->filePath.view());

            if(replaceResult.error.empty()) {

                auto fileId = loc_man.encodeFile(replaceResult.replaced);

                auto module = fileData.module;

                // here since this path begins with '@'
                // we must determine which module it belongs to, we index modules based on 'scope_name:module_name' format
                // this format, we expect users to use for importing something for example -> import "@wakaztahir:socket/connect.ch"
                // sometimes the scope is empty, so user can directly use -> import "@std/fs.ch"
                if(stmt->filePath[0] == '@') {

                    const auto atDirective = path_handler.get_atDirective(stmt->filePath.view());

                    // TODO this should be present in replace result, and that should be renamed to PathResolutionResult
                    const auto found = mod_storage.find_module(atDirective.replaced);
                    if(found) {

                        module = &found->module_scope;

                    } else {

                        // this is an error, because we couldn't determine the module for this file
                        // even though it's path begins with a '@' which means it's external to module

                        // however currently we do allow user to declare an alias for a path using '@'
                        // it could be an alias as well, however checking alias for each import would lead to bad performance

                        // we should do alias for modules instead, because then a module would be resolved by find_module call
                        // and if not, we can error out based on a flag, because build.lab's direct imports can include modules
                        // that haven't been created yet, so we must not error there

                        // we could do also alias for paths inside external modules, we can still resolve the module, and change the
                        // path appropriately, it would require some changes in our path resolver

                    }

                }

                outImports.emplace_back(fileId, module, stmt->filePath.str(), std::move(replaceResult.replaced), stmt->as_identifier.str());

            } else {
                std::cerr <<  rang::fg::red << "error:" << rang::fg::reset <<  " resolving import path '" << stmt->filePath << "' in file '" << fileData.abs_path << "' because " << replaceResult.error << std::endl;
            }

        } else {
            break;
        }

    }

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
    ModuleFileData data(modFileId, chem::string_view(modFile));
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
        const auto import_res = proc.import_chemical_file(result, fileId, abs_path, &inp_source, use_job_allocator);
        if(import_res && abs_path.ends_with("build.lab")) {
            // we inject a get method into every build.lab file
            // it has to be specifically named build.lab so other lab files that are there for just
            // imports don't get injected a get method which needs the build method to work
            // it must also not be the last build.lab file
            auto& typeBuilder = proc.type_builder;
            auto& allocator = use_job_allocator ? proc.job_allocator : proc.mod_allocator;
            const auto parentNode = &result.unit.scope;
            const auto buildFlag = default_build_lab_build_flag(allocator, typeBuilder, parentNode);
            const auto cachedPtr = default_build_lab_cached_ptr(allocator, typeBuilder, parentNode);
            const auto getMethod = default_build_lab_get_method(allocator, typeBuilder, parentNode, buildFlag->name_view(), cachedPtr->name_view());
            auto& nodes = result.unit.scope.body.nodes;
            nodes.emplace_back(buildFlag);
            nodes.emplace_back(cachedPtr);
            nodes.emplace_back(getMethod);
        }
        return import_res;
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
        ASTFileMetaData& fileData,
        bool use_job_allocator
) {

    // import the file into result (lex and parse)
    const auto success = import_file_in_lab(*this, fileData, result, use_job_allocator);
    if(!success) {
        return false;
    }

    // figure out files imported by this file
    std::vector<ASTFileMetaData> imports;
    figure_out_direct_imports(fileData, result.unit.scope.body.nodes, imports);

    // if has imports, we import those files
    if(!imports.empty()) {
        const auto success2 = import_chemical_files_recursive(pool, result.imports, imports, use_job_allocator);
        if(!success2) {
            result.continue_processing = false;
            return false;
        }
    }

    return true;

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
    result.unit.scope.file_path = chem::string_view(result.abs_path);

    auto& unit = result.unit;

    Lexer lexer(std::string(abs_path), *inp_source, &binder, file_allocator);
    std::vector<Token> tokens;

    // actual lexing
    if(options->benchmark) {
        result.lex_benchmark = std::make_unique<BenchmarkResults>();
        result.lex_benchmark->benchmark_begin();
        lexer.getTokens(tokens);
        result.lex_benchmark->benchmark_end();
    } else {
        lexer.getTokens(tokens);
    }

    const auto total_toks = tokens.size();
    if(total_toks == 0) {
        // empty file
        return true;
    } else if(tokens[total_toks - 1].type == TokenType::Unexpected) {
        auto& last = tokens[total_toks - 1];
        lexer.diagnoser.diagnostic(last.value, result.unit.scope.file_path, last.position, last.position, DiagSeverity::Error);
    }

    // move lexer diagnostics
    result.lex_diagnostics = std::move(lexer.diagnoser.diagnostics);

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
    if(options->benchmark) {
        result.parse_benchmark = std::make_unique<BenchmarkResults>();
        result.parse_benchmark->benchmark_begin();
        parser.parse(unit.scope.body.nodes);
        result.parse_benchmark->benchmark_end();
    } else {
        parser.parse(unit.scope.body.nodes);
    }

    // move parser diagnostics
    result.parse_diagnostics = std::move(parser.diagnostics);

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
    std::unique_ptr<BenchmarkResults> bm_results;
    if(options->benchmark) {
        bm_results = std::make_unique<BenchmarkResults>();
        bm_results->benchmark_begin();
    }
    visitor.declare_before_translation(nodes);
    if(options->benchmark) {
        bm_results->benchmark_end();
        print_benchmarks(std::cout, "2cTranslation:declare", bm_results.get());
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
    std::unique_ptr<BenchmarkResults> bm_results;
    if(options->benchmark) {
        bm_results = std::make_unique<BenchmarkResults>();
        bm_results->benchmark_begin();
    }
    visitor.translate_after_declaration(nodes);
    if(options->benchmark) {
        bm_results->benchmark_end();
        print_benchmarks(std::cout, "2cTranslation:translate", abs_path, bm_results.get());
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
    std::unique_ptr<BenchmarkResults> bm_results;
    if(options->benchmark) {
        bm_results = std::make_unique<BenchmarkResults>();
        bm_results->benchmark_begin();
    }
    visitor.declare_and_translate(nodes);
    if(options->benchmark) {
        bm_results->benchmark_end();
        print_benchmarks(std::cout, "2cTranslation", bm_results.get());
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
    for(const auto dep : mod->dependencies) {
        shallow_dedupe_sorted(outMods, modsMap, dep);
    }
    auto found = modsMap.find(mod);
    if(found != modsMap.end() && found->second) {
        outMods.emplace_back(mod);
        found->second = false;
    }
}
void shallow_dedupe_sorted(std::vector<LabModule*>& outMods, std::vector<LabModule*>& inMods) {
    std::unordered_map<LabModule*, bool> direct_mods(inMods.size());
    // save these modules in direct_mods
    for(const auto mod : inMods) {
        direct_mods[mod] = true;
    }
    for(const auto mod : inMods) {
        shallow_dedupe_sorted(outMods, direct_mods, mod);
    }
}

void forward_declare_in_c(ToCAstVisitor& c_visitor, ASTProcessor* proc, LabModule* mod, const char* name) {
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
                throw std::runtime_error("result is null");
            }
        }
    }

    // The second loop deals with declaring files that are present in this module
    // declaring means (only prototypes, no function bodies, struct prototypes...)
    for(auto& file_ptr : module->direct_files) {

        auto& file = *file_ptr.result;
        auto& unit = file.unit;

        // print the benchmark or verbose output received from processing
        if((options->benchmark || options->verbose) && !empty_diags(file)) {
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
            if(file.result != nullptr) {
                auto& body = file.result->unit.scope.body;

#ifdef DEBUG
                c_visitor.debug_comment(chem::string_view(("ExtImplement " + file.abs_path)));
#endif

                external_implement_in_c(c_visitor, body, file.abs_path);
            } else {
                throw std::runtime_error("result is null");
            }
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