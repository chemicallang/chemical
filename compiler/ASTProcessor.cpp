// Copyright (c) Qinetik 2024.

#include "ASTProcessor.h"

#include <memory>
#include "cst/base/CSTConverter.h"
#include "lexer/Lexi.h"
#include "compiler/SymbolResolver.h"
#include "utils/Benchmark.h"
#include <sstream>
#include "utils/Utils.h"

#ifdef COMPILER_BUILD

std::vector<std::unique_ptr<ASTNode>> TranslateC(
        const char *exe_path,
        const char *abs_path,
        const char *resources_path
);

#endif

ASTImportResult concurrent_processor(int id, int job_id, const FlatIGFile& file, ASTProcessor* processor) {
    return processor->import_file(file);
}

void ASTProcessor::prepare(const std::string& path) {

    // preparing the import graph
    if (options->benchmark) {
        BenchmarkResults bm{};
        bm.benchmark_begin();
        result = determine_import_graph(options->exe_path, path);
        bm.benchmark_end();
        std::cout << "[IGGraph] " << bm.representation() << std::endl;
    } else {
        result = determine_import_graph(options->exe_path, path);
    }

    // print errors in ig
    print_errors(&result.root);

    // print the ig
    if (options->print_ig) {
        std::cout << result.root.representation() << std::endl;
    }

}

std::vector<FlatIGFile> ASTProcessor::flat_imports() {
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

void ASTProcessor::sym_res(Scope& scope, bool is_c_file, const std::string& abs_path) {
    auto prev_has_errors = resolver->has_errors;
    if (is_c_file) {
        resolver->override_symbols = true;
        previous = std::move(resolver->errors);
    }
    resolver->current_path = abs_path;
    std::unique_ptr<BenchmarkResults> bm_results;
    if(options->benchmark) {
        bm_results = std::make_unique<BenchmarkResults>();
        bm_results->benchmark_begin();
    }
    scope.declare_top_level(*resolver);
    scope.declare_and_link(*resolver);
    if(options->benchmark) {
        bm_results->benchmark_end();
        std::cout << std::endl << "[SymRes] " << abs_path << " Completed " << bm_results->representation() << std::endl;
    }
    if(!resolver->errors.empty()) {
        resolver->print_errors(abs_path);
        std::cout << std::endl;
    }
    if (is_c_file) {
        resolver->override_symbols = false;
        resolver->errors = std::move(previous);
        resolver->has_errors = prev_has_errors;
    }
}

ASTImportResult ASTProcessor::import_file(const FlatIGFile& file) {

    auto& abs_path = file.abs_path;
    Scope scope(nullptr);
    auto is_c_file = abs_path.ends_with(".h") || abs_path.ends_with(".c");

    std::ostringstream out;
    if(options->benchmark || options->verbose) {
        out << "[Processing] " << abs_path << '\n';
    }

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
        std::fstream stream;
        SourceProvider provider(&stream);
        Lexer lexer(provider, abs_path);
        lexer.init_complete(options->exe_path);
        lexer.isCBIEnabled = options->isCBIEnabled;

        if(options->benchmark) {
            bm_results = std::make_unique<BenchmarkResults>();
            benchLexFile(&lexer, abs_path, *bm_results);
            out << "[Lex]" << " Completed " << bm_results->representation() << '\n';
        } else {
            lexFile(&lexer, abs_path);
        }
        for (const auto &err: lexer.diagnostics) {
            std::cerr << err.representation(abs_path, "Lexer") << std::endl;
        }
        if (options->print_cst) {
            printTokens(lexer.tokens);
        }
        if (lexer.has_errors) {
            return {{ nullptr },false, is_c_file };
        }

        // convert the tokens
        if(options->benchmark) {
            bm_results->benchmark_begin();
        }

        CSTConverter converter(options->is64Bit, options->target_triple);
        converter.no_imports = true;
        converter.isCBIEnabled = options->isCBIEnabled;
        converter.convert(lexer.tokens);
        if(options->benchmark) {
            bm_results->benchmark_end();
            out << "[Cst2Ast]" << " Completed " << ' ' << bm_results->representation() << '\n';
        }
        for (const auto &err: converter.diagnostics) {
            std::cerr << err.representation(abs_path, "Converter") << std::endl;
        }
        scope.nodes = std::move(converter.nodes);
        if (options->print_representation) {
            out << "[Representation]\n" << scope.representation() << '\n';
        }

    }

    std::cout << std::endl << out.str() << std::flush;

    return { std::move(scope), true, is_c_file };

}

void ASTProcessor::end() {

}