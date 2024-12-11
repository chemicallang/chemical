// Copyright (c) Qinetik 2024.

#include "ASTProcessor.h"

#include <memory>
#include "cst/base/CSTConverter.h"
#include "parser/model/CompilerBinder.h"
#include "parser/Parser.h"
#include "compiler/SymbolResolver.h"
#include "preprocess/2c/2cASTVisitor.h"
#include "utils/Benchmark.h"
#include <sstream>
#include "utils/Utils.h"
#include "preprocess/ShrinkingVisitor.h"
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
#include "ast/statements/Import.h"

#ifdef COMPILER_BUILD
#include "compiler/ctranslator/CTranslator.h"
#endif

ASTFileResultExt concurrent_processor(int id, unsigned int file_id, const FlatIGFile& file, ASTProcessor* processor) {
    return processor->import_file(file_id, file.abs_path);
}

std::string ASTProcessorOptions::get_resources_path() {
    if(!resources_path.empty()) return resources_path;
    resources_path = resources_path_rel_to_exe(exe_path);
    if(resources_path.empty()) {
        std::cerr << "[Compiler] Couldn't locate resources path relative to compiler's executable" << std::endl;
    }
    return resources_path;
}

ASTProcessor::ASTProcessor(
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
) : loc_man(loc_man), options(options), resolver(resolver), path_handler(options->exe_path), binder(binder),
    job_allocator(job_allocator), mod_allocator(mod_allocator),
#ifdef COMPILER_BUILD
        translator(translator),
#endif
    file_allocator(file_allocator)

{

}

void put_import_graph(ImportPathHandler& handler, Parser* parser, std::vector<IGFile>& files, const std::vector<std::string>& paths) {
    for (const auto& path : paths) {
        auto local = determine_import_graph(handler, parser, path);
        files.emplace_back(local.root);
    }
}

void put_import_graph(ImportPathHandler& handler, Parser* parser, IGResult& result, const std::vector<std::string>& paths) {
    if(paths.size() == 1) {
        result = determine_import_graph(handler, parser, paths[0]);
    } else {
        for (const auto& path : paths) {
            auto local = determine_import_graph(handler, parser, path);
            result.root.files.emplace_back(local.root);
        }
    }
}

std::vector<FlatIGFile> ASTProcessor::flat_imports_mul(const std::vector<std::string>& c_paths) {

    std::vector<IGFile> files;

    Parser parser(
            0,
            "",
            nullptr,
            loc_man,
            job_allocator,
            mod_allocator,
            resolver->comptime_scope,
            resolver->is64Bit,
            &binder
    );

    // preparing the import graph
    if (options->benchmark) {
        BenchmarkResults bm{};
        bm.benchmark_begin();
        put_import_graph(path_handler, &parser, files, c_paths);
        bm.benchmark_end();
        std::cout << "[IGGraph] " << bm.representation() << std::endl;
    } else {
        put_import_graph(path_handler, &parser, files, c_paths);
    }

    // print errors in ig
    print_errors(files);

    // print the ig
    if (options->print_ig) {
        representation(std::cout, files);
        std::cout << std::endl;
    }

    auto flat_imports = flatten_by_dedupe(files);
    if(options->print_ig) {
        std::cout << "[IGGraph] Flattened" << std::endl;
        for (const auto &file: flat_imports) {
            std::cout << "-- " << file.abs_path << std::endl;
        }
        std::cout << std::endl;
    }

    return flat_imports;
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

void ASTProcessor::determine_mod_imports(
        ctpl::thread_pool& pool,
        std::vector<ASTFileResultNew>& out_files,
        LabModule* module
) {
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
                files.emplace_back(fileId, abs_path, abs_path);
            }
            std::unordered_map<std::string_view, bool> done_files;
            import_chemical_files(pool, "./", out_files, files, done_files);
            return;
        }
        case LabModuleType::ObjFile:
        case LabModuleType::CFile:
        case LabModuleType::CPPFile:
            return;
        case LabModuleType::Directory:
            const auto& dir_path = module->paths[0];
            if (!std::filesystem::exists(dir_path.data()) || !std::filesystem::is_directory(dir_path.data())) {
                std::cerr << "error: directory doesn't exist '" << dir_path << "' for module '" << module->name.data() << '\'' << std::endl;
                return;
            }
            std::vector<std::string> filePaths;
            getFilesInDirectory(filePaths, dir_path.data());
            std::vector<ASTFileMetaData> files;
            for (auto& abs_path: filePaths) {
                auto fileId = loc_man.encodeFile(abs_path);
                files.emplace_back(fileId, abs_path, abs_path);
            }
            std::unordered_map<std::string_view, bool> done_files;
            import_chemical_files(pool, "./", out_files, files, done_files);
            return;
    }
}

std::vector<FlatIGFile> ASTProcessor::determine_mod_imports(LabModule* module) {
    switch(module->type) {
        case LabModuleType::Files:
            if(module->paths.size() == 1) {
                auto cano = canonical_path(module->paths[0].data());
                if(cano.empty()) {
                    std::cerr << "error: couldn't determine canonical path for the module '" << module->paths[0].data() << '\'' << std::endl;
                } else {
                    return flat_imports(cano);
                };
            } else {
                std::vector<std::string> paths;
                for(auto& str : module->paths) {
                    paths.emplace_back(canonical_path(str.data()));
                    if(paths.back().empty()) {
                        std::cerr << "error: couldn't determine canonical path for file '" << str.data() << "' in module '" << module->name << '\'' << std::endl;
                    }
                }
                return flat_imports_mul(paths);
            }
        case LabModuleType::ObjFile:
        case LabModuleType::CFile:
        case LabModuleType::CPPFile:
            return {};
        case LabModuleType::Directory:
            const auto& dir_path = module->paths[0];
            if (!std::filesystem::exists(dir_path.data()) || !std::filesystem::is_directory(dir_path.data())) {
                std::cerr << "error: directory doesn't exist '" << dir_path << "' for module '" << module->name.data() << '\'' << std::endl;
                return {};
            }
            std::vector<std::string> filePaths;
            getFilesInDirectory(filePaths, dir_path.data());
            return flat_imports_mul(filePaths);
    }
}

void ASTProcessor::sym_res(Scope& scope, bool is_c_file, const std::string& abs_path) {
    // doing stuff
    auto prev_has_errors = resolver->has_errors;
    if (is_c_file) {
        previous = std::move(resolver->diagnostics);
    }
    std::unique_ptr<BenchmarkResults> bm_results;
    if(options->benchmark) {
        bm_results = std::make_unique<BenchmarkResults>();
        bm_results->benchmark_begin();
    }
    if(is_c_file) {
        resolver->override_symbols = true;
        for(const auto node : scope.nodes) {
            const auto& id = node->ns_node_identifier();
            if(id.empty()) {
                // TODO handle empty declarations, for example C contains
                // empty enum declarations, where members can be linked directly
                // enum {  Mem1, Mem2 }
            } else {
                resolver->declare(id, node);
            }
        }
        resolver->override_symbols = false;
    } else {
        resolver->resolve_file(scope, abs_path);
    }
    if(options->benchmark) {
        bm_results->benchmark_end();
        print_benchmarks(std::cout, "SymRes", bm_results.get());
    }
    if(!resolver->diagnostics.empty()) {
        resolver->print_diagnostics(abs_path, "SymRes");
    }
    if (is_c_file) {
        resolver->diagnostics = std::move(previous);
        resolver->has_errors = prev_has_errors;
    }
}

void ASTProcessor::print_benchmarks(std::ostream& stream, const std::string& TAG, BenchmarkResults* bm_results) {
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

ASTFileResultNew concurrent_file_importer(
        int id,
        ctpl::thread_pool* pool,
        ASTProcessor* processor,
        ASTFileMetaData& fileData,
        std::unordered_map<std::string_view, bool>& done_files
) {
    return processor->import_chemical_file(*pool, fileData, done_files);
}

void ASTProcessor::import_chemical_files(
        ctpl::thread_pool& pool,
        const std::string_view& base_path,
        std::vector<ASTFileResultNew>& out_files,
        const std::span<ASTFileMetaData>& files,
        std::unordered_map<std::string_view, bool>& done_files
) {

    std::vector<std::future<ASTFileResultNew>> futures;

    // lock the import mutex to sync
    import_mutex.lock();

    // launch all files concurrently
    for(auto& fileData : files) {

        const auto file_id = fileData.file_id;
        const auto& abs_path = fileData.abs_path;
        auto file = std::string_view(abs_path);

        // check whether we have launched this file or not
        auto found = done_files.find(file);
        if(found != done_files.end()) {
            // do the next file
            continue;
        }

        // set the flag for done and launch it
        done_files[file] = true;
        futures.emplace_back(pool.push(concurrent_file_importer, &pool, this, fileData, done_files));

    }

    // unlock the mutex to let go
    import_mutex.unlock();

    // put each file sequentially into the out_files
    for(auto& future : futures) {
        out_files.emplace_back(future.get());
    }

}

ASTFileResultNew ASTProcessor::import_chemical_file(
        ctpl::thread_pool& pool,
        ASTFileMetaData& fileData,
        std::unordered_map<std::string_view, bool>& done_files
) {

    auto result = import_file(fileData.file_id, fileData.abs_path);

    // figure out files imported by this file
    std::vector<ASTFileMetaData> imports;
    auto& file_nodes = result.unit.scope.nodes;
    for(auto node : file_nodes) {
        auto kind = node->kind();
        if(kind == ASTNodeKind::ImportStmt) {
            auto stmt = node->as_import_stmt_unsafe();
            auto replaceResult = path_handler.resolve_import_path(fileData.abs_path, std::string(stmt->filePath));
            if(replaceResult.error.empty()) {
                auto fileId = loc_man.encodeFile(replaceResult.replaced);
                imports.emplace_back(fileId, stmt->filePath, replaceResult.replaced);
            } else {
                std::cerr << "error: resolving import path '" << stmt->filePath << "' in file '" << fileData.abs_path << "'" << std::endl;
            }
        } else {
            break;
        }
    }

    ASTFileResultNew final_result({ result.continue_processing, result.is_c_file }, fileData, {}, {});

    final_result.nodes = std::move(result.unit.scope.nodes);

    import_chemical_files(pool, fileData.abs_path, final_result.imports, imports, done_files);

    return final_result;

}

ASTFileResultExt ASTProcessor::import_chemical_file_new(unsigned int fileId, const std::string_view& abs_path) {

    ASTUnit unit;

    std::unique_ptr<BenchmarkResults> lex_bm;
    std::unique_ptr<BenchmarkResults> parse_bm;

    FileInputSource inp_source(abs_path.data());
    SourceProvider provider2(&inp_source);
    Lexer lexer(std::string(abs_path), provider2, &binder);
    LexUnit lexUnit;

    lexer.getUnit(lexUnit);

    // parse the file
    Parser parser(
            fileId,
            abs_path,
            lexUnit.tokens.data(),
            resolver->comptime_scope.loc_man,
            job_allocator,
            mod_allocator,
            resolver->comptime_scope,
            resolver->is64Bit,
            &binder
    );

    // put the lexing diagnostic into the parser diagnostic for now
    if(!lexUnit.tokens.empty()) {
        auto& last_token = lexUnit.tokens.back();
        if (last_token.type == TokenType::Unexpected) {
            parser.diagnostics.emplace_back(
                    CSTDiagnoser::make_diag("[DEBUG_TRAD_LEXER] unexpected token is at last", abs_path,
                                            last_token.position, last_token.position, DiagSeverity::Warning));
        }
    }
//        if(options->isCBIEnabled) {
//            bind_lexer_cbi(lexer_cbi.get(), &lexer);
//        }

    if(options->benchmark) {
        lex_bm = std::make_unique<BenchmarkResults>();
        lex_bm->benchmark_begin();
        parser.parse(unit.scope.nodes);
        lex_bm->benchmark_end();
    } else {
        parser.parse(unit.scope.nodes);
    }
    if (parser.has_errors) {
        return {ASTFileResult {std::move(unit), std::move(parser.unit), false, false }, std::move(parser.diagnostics), { }, std::move(lex_bm), nullptr };
    }

    // convert the tokens
    if(options->benchmark) {
        parse_bm = std::make_unique<BenchmarkResults>();
        parse_bm->benchmark_begin();
    }

//    CSTConverter converter(
//            fileId,
//            options->is64Bit,
//            resolver->comptime_scope,
//            binder,
//            job_allocator,
//            mod_allocator,
//            file_allocator
//    );
//    converter.convert(parser.unit.tokens);
//    if(options->benchmark) {
//        parse_bm->benchmark_end();
//    }
//    unit = converter.take_unit();

    return {ASTFileResult {std::move(unit), std::move(parser.unit), true, false }, std::move(parser.diagnostics), {}, std::move(lex_bm), std::move(parse_bm) };

}

ASTFileResultExt ASTProcessor::import_file(unsigned int fileId, const std::string_view& abs_path) {

    auto is_c_file = abs_path.ends_with(".h") || abs_path.ends_with(".c");

    if (is_c_file) {

        ASTUnit unit;

        std::unique_ptr<BenchmarkResults> bm;

        if(options->benchmark) {
            bm = std::make_unique<BenchmarkResults>();
            bm->benchmark_begin();
        }

#ifdef COMPILER_BUILD

        translator->translate(
            options->exe_path.c_str(),
            abs_path.data(),
            options->resources_path.c_str()
        );

        unit.scope.nodes = std::move(translator->nodes);

        if(options->benchmark) {
            bm->benchmark_end();
        }

#elif defined(TCC_BUILD) && defined(DEBUG)
        throw std::runtime_error("cannot translate c file as clang api is not available");
#endif

        return {ASTFileResult {std::move(unit), CSTUnit(), true, true }, {},
#ifdef COMPILER_BUILD
                std::move(translator->diagnostics)
#else
                 {}
#endif
                 , nullptr, std::move(bm) };

    } else {

        return import_chemical_file_new(fileId, abs_path);

    }

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
    std::vector<ASTNode*> imported_generics;
    imported_generics.reserve(resolver->imported_generic.size());
    for(auto& node : resolver->imported_generic) {
        imported_generics.emplace_back(node.first);
    }
    visitor.translate(imported_generics);
    visitor.translate(nodes);
    if(options->benchmark) {
        bm_results->benchmark_end();
        std::cout << "[2cTranslation] " << " Completed " << bm_results->representation() << std::endl;
    }
    if(!visitor.diagnostics.empty()) {
        visitor.print_diagnostics(abs_path, "2cTranslation");
        std::cout << std::endl;
    }
    visitor.reset_errors();
}

void ASTProcessor::declare_in_c(
        ToCAstVisitor& visitor,
        Scope& import_res,
        const FlatIGFile& file
) {
    // translating the nodes
//    std::vector<ASTNode*> imported_generics;
//    imported_generics.reserve(resolver->imported_generic.size());
//    for(auto& node : resolver->imported_generic) {
//        imported_generics.emplace_back(node.first);
//    }
//    visitor.translate(imported_generics);
    visitor.declare(import_res.nodes);
    if(!visitor.diagnostics.empty()) {
        visitor.print_diagnostics(file.abs_path, "2cTranslation");
        std::cout << std::endl;
    }
    visitor.reset_errors();
}

void ASTProcessor::shrink_nodes(
        ShrinkingVisitor& shrinker,
        ASTUnit unit,
        const FlatIGFile& file
) {
    shrinker.visit(unit.scope.nodes);
    shrinked_unit[file.abs_path] = std::move(unit);
}