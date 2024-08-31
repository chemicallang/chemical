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
) : options(options), resolver(resolver) {
    if(options->isCBIEnabled) {
        binder = std::make_unique<CompilerBinderTCC>(nullptr, options->exe_path);
        lexer_cbi = std::make_unique<LexerCBI>();
        provider_cbi = std::make_unique<SourceProviderCBI>();
        prep_lexer_cbi(lexer_cbi.get(), provider_cbi.get());
    }
}

void put_import_graph(IGResult& result, const std::string& exe_path, const std::vector<const char*>& paths) {
    IGResult local;
    if(paths.size() == 1) {
        result = determine_import_graph(exe_path, paths[0]);
    } else {
        for (auto path : paths) {
            local = determine_import_graph(exe_path, path);
            result.root.files.emplace_back(local.root);
        }
    }
}

std::vector<FlatIGFile> ASTProcessor::flat_imports_mul(const std::vector<const char*>& paths) {

    IGResult result;

    // preparing the import graph
    if (options->benchmark) {
        BenchmarkResults bm{};
        bm.benchmark_begin();
        put_import_graph(result, options->exe_path, paths);
        bm.benchmark_end();
        std::cout << "[IGGraph] " << bm.representation() << std::endl;
    } else {
        put_import_graph(result, options->exe_path, paths);
    }

    // print errors in ig
    print_errors(&result.root);

    // print the ig
    if (options->print_ig) {
        std::cout << result.root.representation() << std::endl;
    }

    auto flat_imports = result.root.flatten_by_dedupe();
    if(options->print_ig) {
        std::cout << "[IGGraph] Flattened" << std::endl;
        for (const auto &file: flat_imports) {
            std::cout << "-- " << file.abs_path << std::endl;
        }
        std::cout << std::endl;
    }

    return flat_imports;
}

std::vector<FlatIGFile> ASTProcessor::determine_mod_imports(LabModule* module) {
    switch(module->type) {
        case LabModuleType::Files:
            if(module->paths.size() == 1) {
                return flat_imports(module->paths[0].data());
            } else {
                std::vector<const char*> paths;
                for(auto& str : module->paths) {
                    paths.emplace_back(str.data());
                }
                return flat_imports_mul(paths);
            }
        case LabModuleType::ObjFile:
        case LabModuleType::CFile:
            return {};
        case LabModuleType::Directory:
            throw std::runtime_error("NOT YET IMPLEMENTED DIRECTORY THING");
    }
}

void ASTProcessor::dispose_file_symbols(const std::string& abs_path) {
    // dispose symbols of previous file (if any required) before proceeding
    if(!resolver->dispose_file_symbols.empty()) {
        for(const auto& sym : resolver->dispose_file_symbols) {
            if(!resolver->undeclare(sym)) {
                std::cerr << rang::fg::yellow << "[SymRes] unable to un-declare file symbol " << sym << " in file " << abs_path  << rang::fg::reset << std::endl;
            }
        }
        resolver->dispose_file_symbols.clear();
    }
}

void ASTProcessor::sym_res(Scope& scope, bool is_c_file, const std::string& abs_path) {
    // doing stuff
    auto prev_has_errors = resolver->has_errors;
    if (is_c_file) {
        resolver->override_symbols = true;
        previous = std::move(resolver->errors);
    }
    std::unique_ptr<BenchmarkResults> bm_results;
    if(options->benchmark) {
        bm_results = std::make_unique<BenchmarkResults>();
        bm_results->benchmark_begin();
    }
    resolver->imported_generic.clear();
    resolver->file_scope_start();
    scope.link_asynchronously(*resolver);
    for(auto& node : scope.nodes) {
        auto found = resolver->imported_generic.find(node.get());
        if(found != resolver->imported_generic.end()) {
            resolver->imported_generic.erase(found);
        }
    }
    if(options->benchmark) {
        bm_results->benchmark_end();
        print_benchmarks(std::cout, "SymRes", bm_results.get());
    }
    if(!resolver->errors.empty()) {
        resolver->print_errors(abs_path);
    }
    if (is_c_file) {
        resolver->override_symbols = false;
        resolver->errors = std::move(previous);
        resolver->has_errors = prev_has_errors;
    } else {
        // dispose symbols that must be disposed at the end of file (brought by using namespaces)
        dispose_file_symbols(abs_path);
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
    Scope scope(nullptr, nullptr);
    auto is_c_file = abs_path.ends_with(".h") || abs_path.ends_with(".c");

    std::ostringstream out;

    if (is_c_file) {

        if (options->verbose) {
            out << "[IGGraph] Translating C " << abs_path << '\n';
        }

#if defined(COMPILER_BUILD) && defined(CLANG_LIBS)
        scope.nodes = TranslateC(options->exe_path.c_str(), abs_path.c_str(), options->resources_path.c_str());
#else
        throw std::runtime_error("cannot translate c file as clang api is not available");
#endif

    } else {

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
            return { Scope { nullptr, nullptr },false, is_c_file, std::move(out.str()) };
        }

        // convert the tokens
        if(options->benchmark) {
            bm_results->benchmark_begin();
        }

        CSTConverter converter(file.abs_path, options->is64Bit, options->target_triple);
        converter.no_imports = true;
        converter.isCBIEnabled = options->isCBIEnabled;
        converter.convert(lexer.unit.tokens);
        if(options->benchmark) {
            bm_results->benchmark_end();
            print_benchmarks(out, "Cst2Ast", bm_results.get());
        }
        for (const auto &err: converter.diagnostics) {
            err.ansi(std::cerr, abs_path, "Converter") << std::endl;
        }
        scope.nodes = std::move(converter.nodes);
        if (options->print_representation) {
            out << "[Representation]\n" << scope.representation() << '\n';
        }

    }

    return { std::move(scope), true, is_c_file, std::move(out.str()) };

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
        std::cout << "[2cTranslation] " << file.abs_path << " Completed " << bm_results->representation() << std::endl;
    }
    if(!visitor.errors.empty()) {
        visitor.print_errors(file.abs_path);
        std::cout << std::endl;
    }
    visitor.reset_errors();
}

void ASTProcessor::shrink_nodes(
        ShrinkingVisitor& shrinker,
        Scope& result,
        const FlatIGFile& file
) {
    shrinker.visit(result.nodes);
    shrinked_nodes[file.abs_path] = std::move(result.nodes);
}

void ASTProcessor::end() {

}