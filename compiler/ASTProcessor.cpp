// Copyright (c) Qinetik 2024.

#include "ASTProcessor.h"
#include "cst/base/CSTConverter.h"
#include "lexer/Lexi.h"
#include "compiler/SymbolResolver.h"
#include "utils/Benchmark.h"
#include "utils/Utils.h"

#ifdef COMPILER_BUILD

std::vector<std::unique_ptr<ASTNode>> TranslateC(
        const char *exe_path,
        const char *abs_path,
        const char *resources_path
);

#endif

void ASTProcessor::prepare(const std::string& path) {

    // do not create imports for import statements
    converter->no_imports = true;

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
    if(options->verbose) {
        std::cout << "[IGGraph] Flattened" << std::endl;
        for (const auto &file: flat_imports) {
            std::cout << "-- " << file.abs_path << std::endl;
        }
        std::cout << std::endl;
    }
    return flat_imports;
}

ASTImportResult ASTProcessor::import_file(const FlatIGFile &file) {

    auto& abs_path = file.abs_path;
    Scope scope;
    auto is_c_file = abs_path.ends_with(".h") || abs_path.ends_with(".c");

    if (is_c_file) {

        if (options->verbose) {
            std::cout << "[IGGraph] Translating C " << abs_path << std::endl;
        }

#ifdef COMPILER_BUILD
        scope.nodes = TranslateC(options->exe_path.c_str(), abs_path.c_str(), options->resources_path.c_str());
#else
        throw std::runtime_error("cannot translate c file as clang api is not available");
#endif

    } else {

        if (options->verbose) {
            std::cout << "[IGGraph] Begin Compilation " << abs_path << std::endl;
        }

        // lex the file
        lexer->reset();
        lexer->switch_path(abs_path);
        options->benchmark ? benchLexFile(lexer, abs_path) : lexFile(lexer, abs_path);
        for (const auto &err: lexer->errors) {
            std::cerr << err.representation(abs_path, "Lexer") << std::endl;
        }
        if (options->print_cst) {
            printTokens(lexer->tokens);
        }
        if (lexer->has_errors) {
            return {{},false};
        }

        // convert the tokens
        converter->convert(lexer->tokens);
        for (const auto &err: converter->diagnostics) {
            std::cerr << err.representation(abs_path, "Converter") << std::endl;
        }
        scope.nodes = std::move(converter->nodes);
        if (options->print_representation) {
            std::cout << "[Representation]\n" << scope.representation() << std::endl;
        }

    }

    // resolving the symbols
    auto prev_has_errors = resolver->has_errors;
    if (is_c_file) {
        resolver->override_symbols = true;
        previous = std::move(resolver->errors);
    }
    resolver->current_path = abs_path;
    scope.declare_top_level(*resolver);
    scope.declare_and_link(*resolver);
    if (is_c_file) {
        resolver->print_errors();
        resolver->override_symbols = false;
        resolver->errors = std::move(previous);
        resolver->has_errors = prev_has_errors;
    }
    if (resolver->has_errors) {
        return {{}, false};
    }
    return {std::move(scope), true};
}

void ASTProcessor::end() {
    if (!resolver->errors.empty()) {
        std::cout << std::endl;
        resolver->print_errors();
        std::cout << std::endl;
    }
}