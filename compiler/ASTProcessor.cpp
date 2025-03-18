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

ASTProcessor::ASTProcessor(
        ImportPathHandler& pathHandler,
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
) : loc_man(loc_man), options(options), resolver(resolver), path_handler(pathHandler), binder(binder),
    job_allocator(job_allocator), mod_allocator(mod_allocator),
#ifdef COMPILER_BUILD
        translator(translator),
#endif
    file_allocator(file_allocator)

{

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

bool ASTProcessor::empty_diags(ASTFileResultNew& result) {
    return result.lex_diagnostics.empty() && result.parse_diagnostics.empty() && !result.lex_benchmark && !result.parse_benchmark;
}

void ASTProcessor::print_results(ASTFileResultNew& result, const chem::string_view& abs_path, bool benchmark) {
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

void ASTProcessor::determine_mod_imports(
        ctpl::thread_pool& pool,
        std::vector<ASTFileResultNew*>& out_files,
        LabModule* module
) {
    path_handler.module_src_dir_path = "";
    switch(module->type) {
        case LabModuleType::Files: {
            std::vector<ASTFileMetaData> files;
            for (auto& str: module->paths) {
                auto abs_path = canonical_path(str.data());
                if (abs_path.empty()) {
                    std::cerr << "error: couldn't determine canonical path for file '" << str.data() << "' in module '"
                              << module->name << '\'' << std::endl;
                }
                auto fileId = loc_man.encodeFile(abs_path);
                files.emplace_back(fileId, SymbolRange { 0, 0 }, abs_path, abs_path);
            }
            import_chemical_files(pool, out_files, files);
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
            std::vector<ASTFileMetaData> files;
            for (auto& abs_path: filePaths) {
                auto fileId = loc_man.encodeFile(abs_path);
                files.emplace_back(fileId, SymbolRange { 0, 0 }, abs_path, abs_path);
            }
            import_chemical_files(pool, out_files, files);
            return;
    }
}

void ASTProcessor::sym_res_c_file(Scope& scope, const std::string& abs_path) {
    // doing stuff
    auto prev_has_errors = resolver->has_errors;
    previous = std::move(resolver->diagnostics);
    std::unique_ptr<BenchmarkResults> bm_results;
    if(options->benchmark) {
        bm_results = std::make_unique<BenchmarkResults>();
        bm_results->benchmark_begin();
    }
    for(const auto node : scope.nodes) {
        auto id = node->get_located_id();
        if(id) {
            if (id->identifier.empty()) {
                // TODO handle empty declarations, for example C contains
                // empty enum declarations, where members can be linked directly
                // enum {  Mem1, Mem2 }
            } else {
                resolver->declare_overriding(id->identifier, node);
            }
        }
    }
    if(options->benchmark) {
        bm_results->benchmark_end();
        print_benchmarks(std::cout, "SymRes:" + abs_path, bm_results.get());
    }
    if(!resolver->diagnostics.empty()) {
        resolver->print_diagnostics(chem::string_view(abs_path), "SymRes");
    }
    resolver->diagnostics = std::move(previous);
    resolver->has_errors = prev_has_errors;
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
        resolver->print_diagnostics(chem::string_view(abs_path), "SymRes");
    }
    return range;
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
        resolver->print_diagnostics(chem::string_view(abs_path), "SymRes");
    }
}

int ASTProcessor::sym_res_files(std::vector<ASTFileResult*>& files) {
    int i = -1;
    for(auto file_ptr : files) {
        i++;

        auto& file = *file_ptr;
        bool already_imported = shrinked_unit.find(file.abs_path) != shrinked_unit.end();

        if(!already_imported) {
            file.private_symbol_range = sym_res_tld_declare_file(file.unit.scope, file.abs_path);
            // report and clear diagnostics
            if (resolver->has_errors && !options->ignore_errors) {
                std::cerr << rang::fg::red << "couldn't perform job due to errors during symbol resolution" << rang::fg::reset << std::endl;
                return 1;
            }
            resolver->reset_errors();
        }

    }

    for(auto file_ptr : files) {
        auto& file = *file_ptr;
        bool already_imported = shrinked_unit.find(file.abs_path) != shrinked_unit.end();
        if(!already_imported) {
            resolver->link_signature_file(file.unit.scope, file.abs_path, file.private_symbol_range);
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

        auto imported = shrinked_unit.find(file.abs_path);
        bool already_imported = imported != shrinked_unit.end();

        // symbol resolution
        if(!already_imported) {
            sym_res_link_file(file.unit.scope, file.abs_path, file.private_symbol_range);
            if (resolver->has_errors && !options->ignore_errors) {
                std::cerr << rang::fg::red << "couldn't perform job due to errors during symbol resolution" << rang::fg::reset << std::endl;
                return 1;
            }
            resolver->reset_errors();
        }

        // clear everything allocated during symbol resolution of current file
        safe_clear_file_allocator();

    }

    return 0;

}

//void ASTProcessor::sym_res_file(Scope& scope, bool is_c_file, const std::string& abs_path) {
//    // doing stuff
//    auto prev_has_errors = resolver->has_errors;
//    if (is_c_file) {
//        previous = std::move(resolver->diagnostics);
//    }
//    std::unique_ptr<BenchmarkResults> bm_results;
//    if(options->benchmark) {
//        bm_results = std::make_unique<BenchmarkResults>();
//        bm_results->benchmark_begin();
//    }
//    if(is_c_file) {
//        for(const auto node : scope.nodes) {
//            auto id = node->get_located_id();
//            if(id) {
//                if (id->identifier.empty()) {
//                    // TODO handle empty declarations, for example C contains
//                    // empty enum declarations, where members can be linked directly
//                    // enum {  Mem1, Mem2 }
//                } else {
//                    resolver->declare_overriding(id->identifier, node);
//                }
//            }
//        }
//    } else {
//        resolver->resolve_file(scope, abs_path);
//    }
//    if(options->benchmark) {
//        bm_results->benchmark_end();
//        print_benchmarks(std::cout, "SymRes:" + abs_path, bm_results.get());
//    }
//    if(!resolver->diagnostics.empty()) {
//        resolver->print_diagnostics(abs_path, "SymRes");
//    }
//    if (is_c_file) {
//        resolver->diagnostics = std::move(previous);
//        resolver->has_errors = prev_has_errors;
//    }
//}

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

ASTFileResultNew* concurrent_file_importer(
        int id,
        ASTProcessor* processor,
        ctpl::thread_pool& pool,
        ASTFileResultNew* out_file,
        ASTFileMetaData& fileData
) {
    processor->import_chemical_file(*out_file, pool, fileData);
    return out_file;
}

struct future_ptr_union {
    ASTFileResultNew* result = nullptr;
    std::future<ASTFileResultNew*> future;
};

#ifdef DEBUG
#define DEBUG_FUTURE true
#endif

void ASTProcessor::import_chemical_files(
        ctpl::thread_pool& pool,
        std::vector<ASTFileResultNew*>& out_files,
        std::vector<ASTFileMetaData>& files
) {

    std::vector<future_ptr_union> futures;

    // pointer variable to be used inside the for loop
    ASTFileResultNew* out_file;

    // launch all files concurrently
    for(auto& fileData : files) {

        const auto file_id = fileData.file_id;
        const auto& abs_path = fileData.abs_path;
        auto file = std::string_view(abs_path);

        {
            std::lock_guard<std::mutex> guard(import_mutex);
            auto found = cache.find(abs_path);
            if (found != cache.end()) {
//                 std::cout << "not launching file : " << fileData.abs_path << std::endl;
                futures.emplace_back(&found->second);
                continue;
            }

//            std::cout << "launching file : " << fileData.abs_path << std::endl;
            out_file = &cache[abs_path];
        }

        out_file->import_path = fileData.import_path;

#if defined(DEBUG_FUTURE) && DEBUG_FUTURE
        futures.emplace_back(concurrent_file_importer(0, this, pool, out_file, fileData));
#else
        futures.emplace_back(
                nullptr,
                pool.push(concurrent_file_importer, this, std::ref(pool), out_file, fileData)
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

}

void ASTProcessor::import_chemical_file(
        ASTFileResultNew& result,
        ctpl::thread_pool& pool,
        ASTFileMetaData& fileData
) {

    import_file(result, fileData.file_id, fileData.abs_path);

    // figure out files imported by this file
    std::vector<ASTFileMetaData> imports;
    auto& file_nodes = result.unit.scope.nodes;
    for(auto node : file_nodes) {
        auto kind = node->kind();
        if(kind == ASTNodeKind::ImportStmt) {
            auto stmt = node->as_import_stmt_unsafe();
            auto replaceResult = path_handler.resolve_import_path(fileData.abs_path, stmt->filePath.str());
            if(replaceResult.error.empty()) {
                auto fileId = loc_man.encodeFile(replaceResult.replaced);
                imports.emplace_back(fileId, SymbolRange { 0, 0 }, stmt->filePath.str(), std::move(replaceResult.replaced));
            } else {
                std::cerr << "error: resolving import path '" << stmt->filePath << "' in file '" << fileData.abs_path << "' because " << replaceResult.error << std::endl;
            }
        } else {
            break;
        }
    }

    if(!imports.empty()) {

        import_chemical_files(pool, result.imports, imports);

    }

}

void ASTProcessor::import_chemical_file(ASTFileResultNew& result, unsigned int fileId, const std::string_view& abs_path) {

    auto& unit = result.unit;

    std::unique_ptr<BenchmarkResults> lex_bm;
    std::unique_ptr<BenchmarkResults> parse_bm;

    FileInputSource inp_source(abs_path.data());
    if(inp_source.has_error()) {
        result.read_error = inp_source.error_message();
        std::cerr << rang::fg::red << "error: when reading file " << abs_path << " because " << result.read_error << rang::fg::reset << std::endl;
        return;
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
//        if(options->isCBIEnabled) {
//            bind_lexer_cbi(lexer_cbi.get(), &lexer);
//        }

    if(options->benchmark) {
        result.parse_benchmark = std::make_unique<BenchmarkResults>();
        result.parse_benchmark->benchmark_begin();
        parser.parse(unit.scope.nodes);
        result.parse_benchmark->benchmark_end();
    } else {
        parser.parse(unit.scope.nodes);
    }

    result.parse_diagnostics = std::move(parser.diagnostics);

    if(parser.has_errors) {
        result.continue_processing = false;
    }

}

void ASTProcessor::import_file(ASTFileResultNew& result, unsigned int fileId, const std::string_view& abs_path) {

    result.abs_path = abs_path;
    result.file_id = fileId;
    result.private_symbol_range = { 0, 0 };
    result.continue_processing = true;
    result.diCompileUnit = nullptr;

    import_chemical_file(result, fileId, abs_path);

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

        auto imported = shrinked_unit.find(file.abs_path);
        if(imported == shrinked_unit.end()) {
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
            external_declare_in_c(c_visitor, unit.scope, file.abs_path);
        }

    }

    // The second loop deals with declaring files that are present in this module
    // declaring means (only prototypes, no function bodies, struct prototypes...)
    for(auto file_ptr : files) {

        auto& file = *file_ptr;
        auto& result = file;

        if(shrinked_unit.find(file.abs_path) != shrinked_unit.end()) {
            // external module file
            continue;
        }

        // check file exists
        if(file.abs_path.empty()) {
            std::cerr << rang::fg::red << "error: file not found '" << file.import_path << "'" << rang::fg::reset << std::endl;
            return 1;
        }

        if(!result.read_error.empty()) {
            std::cerr << rang::fg::red << "error: when reading file '" << file.abs_path << "' with message '" << result.read_error << "'" << rang::fg::reset << std::endl;
            return 1;
        }

        ASTUnit& unit = file.unit;

        // print the benchmark or verbose output received from processing
        if((options->benchmark || options->verbose) && !empty_diags(result)) {
            std::cout << rang::style::bold << rang::fg::magenta << "[Declare] " << file.abs_path << rang::fg::reset << rang::style::reset << '\n';
            print_results(result, chem::string_view(file.abs_path), options->benchmark);
        }

        // do not continue processing
        if(!result.continue_processing) {
            std::cerr << rang::fg::red << "couldn't perform job due to errors during lexing or parsing file '" << file.abs_path << '\'' << rang::fg::reset << std::endl;
            return 1;
        }

        // translating to c
        declare_before_translation(c_visitor, unit.scope.nodes, file.abs_path);

    }

    // The third loop deals with implementing files that have been imported from other modules
    for(auto file_ptr : files) {

        auto& file = *file_ptr;
        auto& result = file;

        auto imported = shrinked_unit.find(file.abs_path);
        if(imported == shrinked_unit.end()) {
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
            external_implement_in_c(c_visitor, unit.scope, file.abs_path);
            unit.declared_in[module] = true;

            // clear everything we allocated using file allocator to make it re-usable
            safe_clear_file_allocator();

        }

    }

    // The fourth loop deals with generating function bodies present in the current module
    for(auto file_ptr : files) {

        auto& file = *file_ptr;
        auto& result = file;

        if(shrinked_unit.find(file.abs_path) != shrinked_unit.end()) {
            // external module file
            continue;
        }

        ASTUnit& unit = file.unit;
        // translating to c
        translate_after_declaration(c_visitor, unit.scope.nodes, file.abs_path);
        // save the file result, for future retrievals
        shrinked_unit[file.abs_path] = std::move(result.unit);

        // clear everything we allocated using file allocator to make it re-usable
        safe_clear_file_allocator();

    }

    // resetting c visitor to use with another module
    c_visitor.reset();

}