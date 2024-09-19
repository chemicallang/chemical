// Copyright (c) Qinetik 2024.

#include "ASTProcessor.h"

#include <memory>
#include "cst/base/CSTConverter.h"
#include "lexer/Lexi.h"
#include "compiler/SymbolResolver.h"
#include "utils/Benchmark.h"
#include <sstream>
#include "utils/Utils.h"
#include "lexer/model/CompilerBinderTCC.h"
#include "preprocess/ShrinkingVisitor.h"
#include "utils/PathUtils.h"
#include "compiler/lab/LabBuildCompiler.h"
#include "std/chem_string.h"
#include "rang.hpp"
#include <filesystem>

#ifdef COMPILER_BUILD

std::vector<std::unique_ptr<ASTNode>> TranslateC(
        const char *exe_path,
        const char *abs_path,
        const char *resources_path
);

#endif

ASTImportResultExt concurrent_processor(int id, int job_id, const FlatIGFile& file, ASTProcessor* processor) {
    return processor->import_file(file);
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
        SymbolResolver* resolver
) : options(options), resolver(resolver), path_handler(options->exe_path) {
    if(options->isCBIEnabled) {
        // TODO compiler binder has a global interpret scope, however we are using a different compile time scope inside
        // code generation
        binder = std::make_unique<CompilerBinderTCC>(nullptr, options->exe_path);
        lexer_cbi = std::make_unique<LexerCBI>();
        provider_cbi = std::make_unique<SourceProviderCBI>();
        prep_lexer_cbi(lexer_cbi.get(), provider_cbi.get());
    }
}

void put_import_graph(ImportPathHandler& handler, std::vector<IGFile>& files, const std::vector<std::string>& paths) {
    for (const auto& path : paths) {
        auto local = determine_import_graph(handler, path);
        files.emplace_back(local.root);
    }
}

void put_import_graph(ImportPathHandler& handler, IGResult& result, const std::vector<std::string>& paths) {
    if(paths.size() == 1) {
        result = determine_import_graph(handler, paths[0]);
    } else {
        for (const auto& path : paths) {
            auto local = determine_import_graph(handler, path);
            result.root.files.emplace_back(local.root);
        }
    }
}

std::vector<FlatIGFile> ASTProcessor::flat_imports_mul(const std::vector<std::string>& c_paths) {

    std::vector<IGFile> files;

    // preparing the import graph
    if (options->benchmark) {
        BenchmarkResults bm{};
        bm.benchmark_begin();
        put_import_graph(path_handler, files, c_paths);
        bm.benchmark_end();
        std::cout << "[IGGraph] " << bm.representation() << std::endl;
    } else {
        put_import_graph(path_handler, files, c_paths);
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
        resolver->override_symbols = true;
        previous = std::move(resolver->diagnostics);
    }
    std::unique_ptr<BenchmarkResults> bm_results;
    if(options->benchmark) {
        bm_results = std::make_unique<BenchmarkResults>();
        bm_results->benchmark_begin();
    }
    resolver->resolve_file(scope, abs_path);
    if(options->benchmark) {
        bm_results->benchmark_end();
        print_benchmarks(std::cout, "SymRes", bm_results.get());
    }
    if(!resolver->diagnostics.empty()) {
        resolver->print_diagnostics(abs_path, "SymRes");
    }
    if (is_c_file) {
        resolver->override_symbols = false;
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

ASTImportResultExt ASTProcessor::import_file(const FlatIGFile& file) {

    auto& abs_path = file.abs_path;

    auto is_c_file = abs_path.ends_with(".h") || abs_path.ends_with(".c");

    std::ostringstream out;

    if (is_c_file) {

        ASTUnit unit;

        if (options->verbose) {
            out << "[IGGraph] Translating C " << abs_path << '\n';
        }

#if defined(COMPILER_BUILD) && defined(CLANG_LIBS)
        unit.scope.nodes = TranslateC(options->exe_path.c_str(), abs_path.c_str(), options->resources_path.c_str());
#else
        throw std::runtime_error("cannot translate c file as clang api is not available");
#endif

        return { std::move(unit), CSTUnit(), true, true, std::move(out.str()) };

    } else {

        ASTUnit unit;

        if (options->verbose) {
            out << "[IGGraph] Begin Compilation " << abs_path << '\n';
        }

        std::unique_ptr<BenchmarkResults> bm_results;

        // lex the file
        SourceProvider provider(nullptr);
        Lexer lexer(provider, binder.get(), lexer_cbi.get());
        if(options->isCBIEnabled) {
            bind_lexer_cbi(lexer_cbi.get(), provider_cbi.get(), &lexer);
        }

        if(options->benchmark) {
            bm_results = std::make_unique<BenchmarkResults>();
            benchLexFile(&lexer, abs_path, *bm_results);
            print_benchmarks(out, "Lex", bm_results.get());
        } else {
            lexFile(&lexer, abs_path);
        }
        for (const auto &err: lexer.diagnostics) {
            err.ansi(std::cerr, abs_path, "Lexer") << std::endl;
        }
        if (options->print_cst) {
            printTokens(lexer.unit.tokens);
        }
        if (lexer.has_errors) {
            return { std::move(unit), std::move(lexer.unit), false, is_c_file, std::move(out.str()) };
        }

        // convert the tokens
        if(options->benchmark) {
            bm_results->benchmark_begin();
        }

        CSTConverter converter(file.abs_path, options->is64Bit, options->target_triple, resolver->comptime_scope);
        converter.isCBIEnabled = options->isCBIEnabled;
        converter.convert(lexer.unit.tokens);
        if(options->benchmark) {
            bm_results->benchmark_end();
            print_benchmarks(out, "Cst2Ast", bm_results.get());
        }
        for (const auto &err: converter.diagnostics) {
            err.ansi(std::cerr, abs_path, "Converter") << std::endl;
        }
        unit = converter.take_unit();
        if (options->print_representation) {
            out << "[Representation]\n" << unit.scope.representation() << '\n';
        }

        return { std::move(unit), std::move(lexer.unit), true, true, std::move(out.str()) };

    }

}

void ASTProcessor::translate_to_c(
        ToCAstVisitor& visitor,
        Scope& import_res,
        const FlatIGFile& file
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
    visitor.translate(import_res.nodes);
    if(options->benchmark) {
        bm_results->benchmark_end();
        std::cout << "[2cTranslation] " << " Completed " << bm_results->representation() << std::endl;
    }
    if(!visitor.diagnostics.empty()) {
        visitor.print_diagnostics(file.abs_path, "2cTranslation");
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

void ASTProcessor::end() {

}