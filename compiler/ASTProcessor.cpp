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
#include <filesystem>
#include "lexer/Lexer.h"
#include "stream/FileInputSource.h"
#include "ast/base/GlobalInterpretScope.h"
#include "preprocess/ImportPathHandler.h"
#include "ast/statements/Import.h"
#include "compiler/symres/NodeSymbolDeclarer.h"

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

void getFilesInDirectory(std::vector<std::string>& filePaths, const std::string& dirPath) {
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
            getFilesInDirectory(filePaths, entry.path().string());
        }
    }
}

bool ASTProcessor::empty_diags(ASTFileResult& result) {
    return result.lex_diagnostics.empty() && result.parse_diagnostics.empty() && !result.lex_benchmark && !result.parse_benchmark;
}

void ASTProcessor::print_results(ASTFileResult& result, const chem::string_view& abs_path, bool benchmark) {
    CSTDiagnoser::print_diagnostics(result.lex_diagnostics, abs_path, "Lexer");
    CSTDiagnoser::print_diagnostics(result.parse_diagnostics, abs_path, "Parser");
    if(benchmark) {
        if(result.lex_benchmark) {
            ASTProcessor::print_benchmarks(std::cout, "Lexer", result.lex_benchmark.get());
        }
        if(result.parse_benchmark) {
            ASTProcessor::print_benchmarks(std::cout, "Parser", result.parse_benchmark.get());
        }
    }
    std::cout << std::flush;
}

void ASTProcessor::determine_module_files(LabModule* module) {
    auto& files = module->direct_files;
    if(!files.empty()) {
        // it seems the files have already been determined
        return;
    }
    path_handler.module_src_dir_path = "";
    switch(module->type) {
        case LabModuleType::Files: {
            for (auto& str: module->paths) {
                auto abs_path = canonical_path(str.data());
                if (abs_path.empty()) {
                    std::cerr << "error: couldn't determine canonical path for file '" << str.data() << "' in module '"
                              << module->name << '\'' << std::endl;
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
            const auto& dir_path = module->paths[0];
            path_handler.module_src_dir_path = dir_path.to_view();
            if (!std::filesystem::exists(dir_path.data()) || !std::filesystem::is_directory(dir_path.data())) {
                std::cerr << "error: directory doesn't exist '" << dir_path << "' for module '" << module->name.data() << '\'' << std::endl;
                return;
            }
            std::vector<std::string> filePaths;
            getFilesInDirectory(filePaths, dir_path.data());
            for (auto& abs_path: filePaths) {
                auto fileId = loc_man.encodeFile(abs_path);
                files.emplace_back(fileId, &module->module_scope, abs_path, abs_path, "");
            }
            return;
    }
}

bool ASTProcessor::import_module_files(
        ctpl::thread_pool& pool,
        std::vector<ASTFileResult*>& out_files,
        std::vector<ASTFileMetaData>& files,
        LabModule* module,
        bool use_job_allocator
) {
    if(module->type == LabModuleType::Directory) {
        path_handler.module_src_dir_path = module->paths[0].to_view();
    } else {
        path_handler.module_src_dir_path = "";
    }
    return import_chemical_files(pool, out_files, files, use_job_allocator);
}

SymbolRange ASTProcessor::sym_res_tld_declare_file(Scope& scope, const std::string& abs_path) {
    // doing stuff
    auto prev_has_errors = resolver->has_errors;
    std::unique_ptr<BenchmarkResults> bm_results;
    if(options->benchmark) {
        bm_results = std::make_unique<BenchmarkResults>();
        bm_results->benchmark_begin();
    }
    const auto range = resolver->tld_declare_file(scope, abs_path);
    if(options->benchmark) {
        bm_results->benchmark_end();
        print_benchmarks(std::cout, "SymRes:declare", abs_path, bm_results.get());
    }
    if(!resolver->diagnostics.empty()) {
        resolver->print_diagnostics(chem::string_view(abs_path), "SymRes:declare");
    }
    return range;
}

void ASTProcessor::sym_res_link_sig_file(Scope& scope, const std::string& abs_path, const SymbolRange& range) {
    // doing stuff
    auto prev_has_errors = resolver->has_errors;
    std::unique_ptr<BenchmarkResults> bm_results;
    if(options->benchmark) {
        bm_results = std::make_unique<BenchmarkResults>();
        bm_results->benchmark_begin();
    }
    resolver->link_signature_file(scope, abs_path, range);
    if(options->benchmark) {
        bm_results->benchmark_end();
        print_benchmarks(std::cout, "SymRes:link_sig", abs_path, bm_results.get());
    }
    if(!resolver->diagnostics.empty()) {
        resolver->print_diagnostics(chem::string_view(abs_path), "SymRes:link_sig");
    }
}

void ASTProcessor::sym_res_link_file(Scope& scope, const std::string& abs_path, const SymbolRange& range) {
    // doing stuff
    auto prev_has_errors = resolver->has_errors;
    std::unique_ptr<BenchmarkResults> bm_results;
    if(options->benchmark) {
        bm_results = std::make_unique<BenchmarkResults>();
        bm_results->benchmark_begin();
    }
    resolver->link_file(scope, abs_path, range);
    if(options->benchmark) {
        bm_results->benchmark_end();
        print_benchmarks(std::cout, "SymRes:link", abs_path, bm_results.get());
    }
    if(!resolver->diagnostics.empty()) {
        resolver->print_diagnostics(chem::string_view(abs_path), "SymRes:link");
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

        file.private_symbol_range = sym_res_tld_declare_file(file.unit.scope.body, file.abs_path);

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

        sym_res_link_sig_file(file.unit.scope.body, file.abs_path, file.private_symbol_range);
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

        sym_res_link_file(file.unit.scope.body, file.abs_path, file.private_symbol_range);
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

ASTFileResult* chemical_file_concurrent_importer(
        int id,
        ASTProcessor* processor,
        ctpl::thread_pool& pool,
        ASTFileResult* out_file,
        ASTFileMetaData& fileData,
        bool use_job_allocator
) {
    processor->import_chemical_file(*out_file, pool, fileData, use_job_allocator);
    return out_file;
}

struct future_ptr_union {
    ASTFileResult* result = nullptr;
    std::future<ASTFileResult*> future;
};

#ifdef DEBUG
#define DEBUG_FUTURE false
#endif

bool ASTProcessor::import_chemical_files(
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
        futures.emplace_back(chemical_file_concurrent_importer(0, this, pool, out_file, fileData, use_job_allocator));
#else
        futures.emplace_back(
                nullptr,
                pool.push(chemical_file_concurrent_importer, this, std::ref(pool), out_file, fileData, use_job_allocator)
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
                continue;
            }

            // resolve the import path of this import statement
            auto replaceResult = path_handler.resolve_import_path(fileData.abs_path, stmt->filePath.str());

            if(replaceResult.error.empty()) {

                auto fileId = loc_man.encodeFile(replaceResult.replaced);

                auto module = fileData.module;

                // here since this path begins with '@'
                // we must determine which module it belongs to, we index modules based on 'scope_name:module_name' format
                // this format, we expect users to use for importing something for example -> import "@wakaztahir:socket/connect.ch"
                // sometimes the scope is empty, so user can directly use -> import "@std/fs.ch"
                if(stmt->filePath[0] == '@') {

                    const auto atDirective = path_handler.get_atDirective(stmt->filePath.str());

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

bool ASTProcessor::import_chemical_file(
        ASTFileResult& result,
        ctpl::thread_pool& pool,
        ASTFileMetaData& fileData,
        bool use_job_allocator
) {

    // import the file into result (lex and parse)
    const auto success = import_file(result, fileData.file_id, fileData.abs_path, use_job_allocator);
    if(!success) {
        return false;
    }

    // figure out files imported by this file
    std::vector<ASTFileMetaData> imports;
    figure_out_direct_imports(fileData, result.unit.scope.body.nodes, imports);

    // if has imports, we import those files
    if(!imports.empty()) {
        const auto success2 = import_chemical_files(pool, result.imports, imports, use_job_allocator);
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
        bool use_job_allocator
) {

    auto& unit = result.unit;

    FileInputSource inp_source(abs_path.data());
    if(inp_source.has_error()) {
        result.continue_processing = false;
        result.read_error = inp_source.error_message();
        std::cerr << rang::fg::red << "error: when reading file " << abs_path;
        if(!result.read_error.empty()) {
            std::cerr << " because " << result.read_error;
        }
        std::cerr << rang::fg::reset << std::endl;
        return false;
    }

    Lexer lexer(std::string(abs_path), &inp_source, &binder, file_allocator);
    std::vector<Token> tokens;

    if(options->benchmark) {
        result.lex_benchmark = std::make_unique<BenchmarkResults>();
        result.lex_benchmark->benchmark_begin();
        lexer.getTokens(tokens);
        result.lex_benchmark->benchmark_end();
    } else {
        lexer.getTokens(tokens);
    }

    // lexer doesn't have diagnostics
    // result.lex_diagnostics = {};

    // parse the file
    Parser parser(
            fileId,
            abs_path,
            tokens.data(),
            resolver->comptime_scope.loc_man,
            job_allocator,
            use_job_allocator ? job_allocator : mod_allocator,
            resolver->is64Bit,
            &binder
    );

    // put the lexing diagnostic into the parser diagnostic for now
    if(!tokens.empty()) {
        auto& last_token = tokens.back();
        if (last_token.type == TokenType::Unexpected) {
            parser.diagnostics.emplace_back(
                    CSTDiagnoser::make_diag("[DEBUG_TRAD_LEXER] unexpected token is at last", chem::string_view(abs_path), last_token.position, last_token.position, DiagSeverity::Warning));
        }
    }

    // setting file scope as parent of all nodes parsed
    parser.parent_node = &result.unit.scope;

    if(options->benchmark) {
        result.parse_benchmark = std::make_unique<BenchmarkResults>();
        result.parse_benchmark->benchmark_begin();
        parser.parse(unit.scope.body.nodes);
        result.parse_benchmark->benchmark_end();
    } else {
        parser.parse(unit.scope.body.nodes);
    }

    result.parse_diagnostics = std::move(parser.diagnostics);

    if(parser.has_errors) {
        result.continue_processing = false;
        return false;
    } else {
        return true;
    }

}

bool ASTProcessor::import_chemical_mod_file(
        LabBuildCompiler& compiler,
        ASTFileResult& result,
        ModuleFileData& data,
        unsigned int fileId,
        const std::string_view& abs_path
) {

    auto& unit = result.unit;
    const auto options = compiler.options;
    const auto is64Bit = options->is64Bit;

    std::unique_ptr<BenchmarkResults> lex_bm;
    std::unique_ptr<BenchmarkResults> parse_bm;

    FileInputSource inp_source(abs_path.data());
    if(inp_source.has_error()) {
        result.continue_processing = false;
        result.read_error = inp_source.error_message();
        std::cerr << rang::fg::red << "error: when reading file " << abs_path;
        if(!result.read_error.empty()) {
            std::cerr << " because " << result.read_error;
        }
        std::cerr << rang::fg::reset << std::endl;
        return false;
    }

    Lexer lexer(std::string(abs_path), &inp_source, &compiler.binder, *compiler.file_allocator);
    std::vector<Token> tokens;

    if(options->benchmark) {
        result.lex_benchmark = std::make_unique<BenchmarkResults>();
        result.lex_benchmark->benchmark_begin();
        lexer.getTokens(tokens);
        result.lex_benchmark->benchmark_end();
    } else {
        lexer.getTokens(tokens);
    }

    // lexer doesn't have diagnostics
    // result.lex_diagnostics = {};

    // parse the file
    Parser parser(
            fileId,
            abs_path,
            tokens.data(),
            compiler.loc_man,
            *compiler.job_allocator,
            *compiler.mod_allocator,
            is64Bit,
            &compiler.binder
    );

    // put the lexing diagnostic into the parser diagnostic for now
    if(!tokens.empty()) {
        auto& last_token = tokens.back();
        if (last_token.type == TokenType::Unexpected) {
            parser.diagnostics.emplace_back(
                    CSTDiagnoser::make_diag("[DEBUG_TRAD_LEXER] unexpected token is at last", chem::string_view(abs_path), last_token.position, last_token.position, DiagSeverity::Warning));
        }
    }

    // setting file scope as parent of all nodes parsed
    parser.parent_node = &result.unit.scope;

    if(options->benchmark) {
        result.parse_benchmark = std::make_unique<BenchmarkResults>();
        result.parse_benchmark->benchmark_begin();
        parser.parseModuleFile(unit.scope.body.nodes, data);
        result.parse_benchmark->benchmark_end();
    } else {
        parser.parseModuleFile(unit.scope.body.nodes, data);
    }

    result.parse_diagnostics = std::move(parser.diagnostics);

    if(parser.has_errors) {
        result.continue_processing = false;
        return false;
    } else {
        return true;
    }

}

// this function cannot be used as a replacement for .mod or .lab files
// because a module won't import all the files from external module, only a few
// and we can't determine which modules it depends on because one of the non-imported files may
// have dependencies on other modules (in other words, an import tree in our language doesn't tell us the complete module graph)
void ASTProcessor::figure_out_module_dependency_based_on_import(
        ASTFileResult& imported,
        std::vector<BuildLabModuleDependency>& dependencies
) {

    auto& result = imported;

    // then we compile the entire module
    if(result.import_path[0] == '@' && !result.import_path.ends_with(".lab")) {

        // module identifier for the import path
        auto modIdentifier = path_handler.get_mod_identifier_from_import_path(result.import_path);

        // got the dir path for the module
        auto dir_path = path_handler.resolve_lib_dir_path(modIdentifier.scope_name, modIdentifier.module_name);

        if(dir_path.error.empty()) {

            // if user is importing a file from the module
            if (result.import_path.ends_with(".ch")) {

                // now we put this module dependency in the vector
                // because this module needs to be built before this file can be imported
                for(auto& dep : dependencies) {
                    if(dep.mod_name.to_chem_view() == modIdentifier.module_name && dep.scope_name.to_chem_view() == modIdentifier.scope_name) {
                        return;
                    }
                }

                dependencies.emplace_back(std::move(dir_path.replaced), chem::string(modIdentifier.scope_name), chem::string(modIdentifier.module_name));

            } else {

                // now we must check if it contains a build.lab
                const auto child_path = resolve_rel_child_path_str(dir_path.replaced, "build.lab");
                if (std::filesystem::exists(child_path)) {

                    // if it contains a build.lab, we would change the absolute path to that build.lab
                    // so that build.lab gets imported (lexed and parsed, no invocation to build method, which would be done manually by the user)
                    result.abs_path = child_path;

                    // import should be done with a 'as' identifier because without it build.lab cannot be imported
                    // (conflict: contains build method with same name) however user could provide a library with a build.lab that doesn't contain
                    // a build method named 'build', so we don't enforce that as_identifier be used

                } else {

                    // however the module doesn't contain build.lab (which means there's no build function)
                    // so we produce an error, because we cannot import that module directly like this
                    std::cerr << "[lab] " << rang::fg::red << "error:" << rang::fg::reset << "cannot import directory '" << result.import_path << "' without a 'build.lab'" << std::endl;

                }

            }

        } else {

            std::cerr << "[lab] " << rang::fg::red << "error:" << rang::fg::reset << "couldn't determine the library directory for '" << result.import_path << "'" << std::endl;

        }

    }

}

bool ASTProcessor::import_file(ASTFileResult& result, unsigned int fileId, const std::string_view& abs_path, bool use_job_allocator) {

    result.abs_path = abs_path;
    result.file_id = fileId;
    result.private_symbol_range = { 0, 0 };
    result.continue_processing = true;
    result.diCompileUnit = nullptr;
    result.unit.scope.file_path = chem::string_view(result.abs_path);

    return import_chemical_file(result, fileId, abs_path, use_job_allocator);

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

int ASTProcessor::translate_module(
    ToCAstVisitor& c_visitor,
    LabModule* module
) {

    // NOTE: here's the design guidelines for translating a module
    // please note that not all guidelines are implemented in this function, some are implemented in
    // the visitor, these only protect from referencing any struct from any module
    // 1 - forward declare all the structs/unions/variants generic or non generic of both modules
    // - before proceeding, now every function in both modules can reference any struct as a pointer
    // - since we only use pointer types in functions, all functions can be declared
    // 2 - declare top level with early declare composed variables and inherited types along with functions, all at once,
    // - do not early declare function params and return type because function types will use only pointer and we have forward declared them
    // - early declare var init type, typealias actual type (do not use canonical type, because if its a typealias, that probably also early declared itself)
    // 3 - declare external module and internal module, before proceeding to implementing any generics
    // - because external module generic can compose the struct of a internal module
    // - we just make a single has_declared check before declaring struct definition, so if early declare has done it, we don't define it twice
    // 4 - now all the structs are declared, functions are declared, only function implementations are remaining
    // - so yeah that's what we will do, without checking, just implement all the functions of external
    //               - or internal modules, implement only the remaining generics


    // let's create a flat vector of direct dependencies, that we want to process
    std::vector<LabModule*> dependencies;
    shallow_dedupe_sorted(dependencies, module->dependencies);

    // forward declare dependencies & dependencies of dependencies & current module
    for(const auto dep1 : dependencies) {
        for(const auto dep : dep1->dependencies) {
            forward_declare_in_c(c_visitor, this, dep, "Ext2FwdDeclare");
        }
        forward_declare_in_c(c_visitor, this, dep1, "ExtFwdDeclare");
    }
    forward_declare_in_c(c_visitor, this, module, "FwdDeclare");

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
            print_results(file, chem::string_view(file.abs_path), options->benchmark);
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