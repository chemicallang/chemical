// Copyright (c) Chemical Language Foundation 2025.

#include "ASTProcessor.h"

#include <memory>
#include "parser/model/CompilerBinder.h"
#include "parser/Parser.h"
#include "compiler/SymbolResolver.h"
#include "preprocess/2c/2cASTVisitor.h"
#include "utils/Benchmark.h"
#include <sstream>
#include "utils/PathUtils.h"
#include "compiler/lab/LabBuildCompiler.h"
#include "std/chem_string.h"
#include "rang.hpp"
#include "integration/cbi/bindings/CBI.h"
#include "preprocess/RepresentationVisitor.h"
#include "ast/structures/FunctionDeclaration.h"
#include <filesystem>
#include "lexer/Lexer.h"
#include "stream/FileInputSource.h"
#include "ast/base/GlobalInterpretScope.h"
#include "preprocess/ImportPathHandler.h"
#include "ast/statements/Import.h"

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
            filePaths.emplace_back(canonical_path(entry.path().string()));
            if(filePaths.back().empty()) {
                // will not happen most of the time, since OS is providing us the paths
                std::cerr << "error: couldn't determine canonical path for the file '" << entry.path().string() << '\'' << std::endl;
                filePaths.pop_back();
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

void ASTProcessor::determine_mod_files(
        std::vector<ASTFileMetaData>& files,
        LabModule* module
) {
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

bool ASTProcessor::import_mod_files(
        ctpl::thread_pool& pool,
        std::vector<ASTFileResult*>& out_files,
        std::vector<ASTFileMetaData>& files,
        LabModule* module
) {
    if(module->type == LabModuleType::Directory) {
        path_handler.module_src_dir_path = module->paths[0].to_view();
    } else {
        path_handler.module_src_dir_path = "";
    }
    return import_chemical_files(pool, out_files, files);
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

int ASTProcessor::sym_res_files(std::vector<ASTFileResult*>& files) {

    // declare symbols for all files once in the module
    int i = -1;
    for(auto file_ptr : files) {
        i++;

        auto& file = *file_ptr;
        bool already_imported = compiled_units.find(file.abs_path) != compiled_units.end();

        if(!already_imported) {
            file.private_symbol_range = sym_res_tld_declare_file(file.unit.scope.body, file.abs_path);
            // report and clear diagnostics
            if (resolver->has_errors && !options->ignore_errors) {
                std::cerr << rang::fg::red << "couldn't perform job due to errors during symbol resolution" << rang::fg::reset << std::endl;
                return 1;
            }
            resolver->reset_errors();
        }

    }

    // link the signature of the files
    for(auto file_ptr : files) {
        auto& file = *file_ptr;
        bool already_imported = compiled_units.find(file.abs_path) != compiled_units.end();
        if(!already_imported) {
            sym_res_link_sig_file(file.unit.scope.body, file.abs_path, file.private_symbol_range);
            // report and clear diagnostics
            if (resolver->has_errors && !options->ignore_errors) {
                std::cerr << rang::fg::red << "couldn't perform job due to errors during symbol resolution" << rang::fg::reset << std::endl;
                return 1;
            }
            resolver->reset_errors();
        }
    }

    // sequentially symbol resolve all the files in the module
    for(auto file_ptr : files) {

        auto& file = *file_ptr;

        auto imported = compiled_units.find(file.abs_path);
        bool already_imported = imported != compiled_units.end();

        // symbol resolution
        if(!already_imported) {
            sym_res_link_file(file.unit.scope.body, file.abs_path, file.private_symbol_range);
            if (resolver->has_errors && !options->ignore_errors) {
                std::cerr << rang::fg::red << "couldn't perform job due to errors during symbol resolution" << rang::fg::reset << std::endl;
                return 1;
            }
            resolver->reset_errors();
        }

        // clear everything allocated during symbol resolution of current file
        safe_clear_file_allocator();

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

    return 0;

}

int ASTProcessor::sym_res_module(std::vector<ASTFileResult*>& mod_files) {
    const auto mod_index = resolver->module_scope_start();
    const auto res = sym_res_files(mod_files);
    resolver->module_scope_end_drop(mod_index);
    resolver->stored_file_symbols.clear();
    return res;
}

int ASTProcessor::sym_res_module_drop(std::vector<ASTFileResult*>& mod_files) {
    const auto mod_index = resolver->module_scope_start();
    const auto res = sym_res_files(mod_files);
    resolver->module_scope_end_drop(mod_index);
    resolver->stored_file_symbols.clear();
    return res;
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
        ASTFileMetaData& fileData
) {
    processor->import_chemical_file(*out_file, pool, fileData);
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
                futures.emplace_back(&found->second);
                continue;
            }

//            std::cout << "launching file : " << fileData.abs_path << std::endl;
            // we must try to store chem::string_view into the fileData, from the beginning
            cache.emplace(abs_path, ASTFileResult(file_id, "", fileData.module));
            out_file = &cache.find(abs_path)->second;
        }

        // copy the metadata into it
        *static_cast<ASTFileMetaData*>(out_file) = fileData;

#if defined(DEBUG_FUTURE) && DEBUG_FUTURE
        futures.emplace_back(chemical_file_concurrent_importer(0, this, pool, out_file, fileData));
#else
        futures.emplace_back(
                nullptr,
                pool.push(chemical_file_concurrent_importer, this, std::ref(pool), out_file, fileData)
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
        ASTFileMetaData& fileData
) {

    // import the file into result (lex and parse)
    const auto success = import_file(result, fileData.file_id, fileData.abs_path);
    if(!success) {
        return false;
    }

    // figure out files imported by this file
    std::vector<ASTFileMetaData> imports;
    figure_out_direct_imports(fileData, result.unit.scope.body.nodes, imports);

    // if has imports, we import those files
    if(!imports.empty()) {
        const auto success2 = import_chemical_files(pool, result.imports, imports);
        if(!success2) {
            result.continue_processing = false;
            return false;
        }
    }

    return true;

}

bool ASTProcessor::import_chemical_file(ASTFileResult& result, unsigned int fileId, const std::string_view& abs_path) {

    auto& unit = result.unit;

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
            parse_on_job_allocator ? job_allocator : mod_allocator,
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

bool ASTProcessor::import_chemical_mod_file(ASTFileResult& result, unsigned int fileId, const std::string_view& abs_path) {

    auto& unit = result.unit;

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
            mod_allocator,
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
        parser.parseModuleFile(unit.scope.body.nodes);
        result.parse_benchmark->benchmark_end();
    } else {
        parser.parseModuleFile(unit.scope.body.nodes);
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
        ASTFileMetaData& imported,
        ASTFileMetaData& importer,
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
                dependencies.emplace_back(std::move(dir_path.replaced), &imported, modIdentifier.scope_name, modIdentifier.module_name);

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

bool ASTProcessor::import_file(ASTFileResult& result, unsigned int fileId, const std::string_view& abs_path) {

    result.abs_path = abs_path;
    result.file_id = fileId;
    result.private_symbol_range = { 0, 0 };
    result.continue_processing = true;
    result.diCompileUnit = nullptr;
    result.unit.scope.file_path = chem::string_view(result.abs_path);

    return import_chemical_file(result, fileId, abs_path);

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
    // translating the nodes
//    std::vector<ASTNode*> imported_generics;
//    imported_generics.reserve(resolver->imported_generic.size());
//    for(auto& node : resolver->imported_generic) {
//        imported_generics.emplace_back(node.first);
//    }
//    visitor.translate(imported_generics);
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

int ASTProcessor::translate_module(
    ToCAstVisitor& c_visitor,
    LabModule* module,
    std::vector<ASTFileResult*>& files
) {

    // The first loop deals with declaring files that have been imported from other modules
    // declaring means (only prototypes, no function bodies, struct prototypes...)
    for(auto file_ptr : files) {

        auto& file = *file_ptr;
        auto& result = file;

        auto imported = compiled_units.find(file.abs_path);
        if(imported == compiled_units.end()) {
            // not external module file
            continue;
        }

        ASTUnit& unit = imported->second;

        // print the benchmark or verbose output received from processing
        if((options->benchmark || options->verbose) && !empty_diags(result)) {
            std::cout << rang::style::bold << rang::fg::magenta << "[ExtDeclare] " << file.abs_path << rang::fg::reset << rang::style::reset << '\n';
        }

        auto declared_in = unit.declared_in.find(module);
        if(declared_in == unit.declared_in.end()) {
            // this is probably a different module, so we'll declare the file (if not declared)
            external_declare_in_c(c_visitor, unit.scope.body, file.abs_path);
        }

    }

    // The second loop deals with declaring files that are present in this module
    // declaring means (only prototypes, no function bodies, struct prototypes...)
    for(auto file_ptr : files) {

        auto& file = *file_ptr;
        auto& result = file;

        if(compiled_units.find(file.abs_path) != compiled_units.end()) {
            // external module file
            continue;
        }

        ASTUnit& unit = file.unit;

        // print the benchmark or verbose output received from processing
        if((options->benchmark || options->verbose) && !empty_diags(result)) {
            std::cout << rang::style::bold << rang::fg::magenta << "[Declare] " << file.abs_path << rang::fg::reset << rang::style::reset << '\n';
            print_results(result, chem::string_view(file.abs_path), options->benchmark);
        }

        // translating to c
        declare_before_translation(c_visitor, unit.scope.body.nodes, file.abs_path);

    }

    // The third loop deals with implementing files that have been imported from other modules
    for(auto file_ptr : files) {

        auto& file = *file_ptr;
        auto& result = file;

        auto imported = compiled_units.find(file.abs_path);
        if(imported == compiled_units.end()) {
            // not external module file
            continue;
        }

        ASTUnit& unit = imported->second;

        // print the benchmark or verbose output received from processing
        if((options->benchmark || options->verbose) && !empty_diags(result)) {
            std::cout << rang::style::bold << rang::fg::magenta << "[ExtImplement] " << file.abs_path << rang::fg::reset << rang::style::reset << '\n';
        }

        auto declared_in = unit.declared_in.find(module);
        if(declared_in == unit.declared_in.end()) {
            // this is probably a different module, so we'll declare the file (if not declared)
            external_implement_in_c(c_visitor, unit.scope.body, file.abs_path);
            unit.declared_in[module] = true;

            // clear everything we allocated using file allocator to make it re-usable
            safe_clear_file_allocator();

        }

    }

    // The fourth loop deals with generating function bodies present in the current module
    for(auto file_ptr : files) {

        auto& file = *file_ptr;
        auto& result = file;

        if(compiled_units.find(file.abs_path) != compiled_units.end()) {
            // external module file
            continue;
        }

        ASTUnit& unit = file.unit;
        // translating to c
        translate_after_declaration(c_visitor, unit.scope.body.nodes, file.abs_path);
        // save the file result, for future retrievals
        compiled_units.emplace(file.abs_path, result.unit);

        // clear everything we allocated using file allocator to make it re-usable
        safe_clear_file_allocator();

    }

    // resetting c visitor to use with another module
    c_visitor.reset();

    // return for success
    return 0;

}