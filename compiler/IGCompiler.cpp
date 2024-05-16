// Copyright (c) Qinetik 2024.

#include "IGCompiler.h"
#include "preprocess/ImportGraphMaker.h"
#include "utils/Benchmark.h"
#include "compiler/Codegen.h"
#include "lexer/Lexi.h"
#include "utils/Utils.h"
#include "cst/base/CSTConverter.h"
#include "SymbolResolver.h"
#include <iostream>
#include <utility>
#include <functional>

std::vector<std::unique_ptr<ASTNode>>
TranslateC(const char *exe_path, const char *abs_path, const char *resources_path);

IGCompilerOptions::IGCompilerOptions(
        std::string exe_path,
        std::string target_triple,
        bool is64Bit
) : exe_path(std::move(exe_path)),
    target_triple(std::move(target_triple)),
    is64Bit(is64Bit) {

}

bool compile(Codegen *gen, const std::string &path, IGCompilerOptions *options) {

    // preparing the import graph
    IGResult result;
    if (options->benchmark) {
        BenchmarkResults bm{};
        bm.benchmark_begin();
        result = determine_import_graph(options->exe_path, path);
        bm.benchmark_end();
        std::cout << "[IGGraph] " << bm.representation() << std::endl;
    } else {
        result = determine_import_graph(options->exe_path, path);
    }

    // print errors
    if (!result.errors.empty()) {
        for (auto &err: result.errors) {
            std::cout << err.ansi_representation(err.doc_url.value(), "IGGraph") << std::endl;
        }
    }

    // print the ig
    if (options->print_ig) {
        std::cout << result.root.representation() << std::endl;
    }


    // The nodes that will be retained during compilation
    std::vector<std::vector<std::unique_ptr<ASTNode>>> file_nodes;

    // creating the lexer
    std::fstream file_stream;
    SourceProvider provider(file_stream);
    Lexer lexer(provider, path);

    // the cst converter
    CSTConverter converter(options->is64Bit);
    converter.no_imports = true; // do not create imports for import statements

    // creating symbol resolver
    SymbolResolver resolver(options->exe_path, path, options->is64Bit);
    resolver.benchmark = options->benchmark;

    // The imported map, when a file is imported, it set's it's absolute path true in this map
    // to avoid re-importing files
    std::unordered_map<std::string, bool> imported;

    // beginning
    gen->compile_begin();

    std::vector<ASTDiag> previous = {};

    auto flat_imports = result.root.flatten_by_dedupe();

    if(options->verbose) {
        std::cout << "[IGGraph] Flattened" << std::endl;
        for (const auto &file: flat_imports) {
            std::cout << "-- " << file.abs_path << std::endl;
        }
        std::cout << std::endl;
    }

    bool compile_result = true;

    for(const auto& file : flat_imports) {

        auto& abs_path = file.abs_path;
        Scope scope;
        auto is_c_file = abs_path.ends_with(".h") || abs_path.ends_with(".c");

        if (is_c_file) {

            if (options->verbose) {
                std::cout << "[IGGraph] Translating C " << abs_path << std::endl;
            }

            scope.nodes = TranslateC(gen->curr_exe_path.c_str(), abs_path.c_str(), gen->resources_dir.c_str());

        } else {

            if (options->verbose) {
                std::cout << "[IGGraph] Begin Compilation " << abs_path << std::endl;
            }

            // lex the file
            lexer.switch_path(abs_path);
            options->benchmark ? benchLexFile(&lexer, abs_path) : lexFile(&lexer, abs_path);
            for (const auto &err: lexer.errors) {
                std::cerr << err.representation(abs_path, "Lexer") << std::endl;
            }
            if (options->print_cst) {
                printTokens(lexer.tokens);
            }
            if (lexer.has_errors) {
                compile_result = false;
                break;
            }

            // convert the tokens
            converter.convert(lexer.tokens);
            for (const auto &err: converter.diagnostics) {
                std::cerr << err.representation(path, "Converter") << std::endl;
            }
            scope.nodes = std::move(converter.nodes);
            if (options->print_representation) {
                std::cout << "[Representation]\n" << scope.representation() << std::endl;
            }

            // clear the lexer tokens
            lexer.tokens.clear();

        }

        // resolving the symbols
        auto prev_has_errors = resolver.has_errors;
        if (is_c_file) {
            resolver.override_symbols = true;
            previous = std::move(resolver.errors);
        }
        resolver.current_path = abs_path;
        scope.declare_top_level(resolver);
        scope.declare_and_link(resolver);
        if (is_c_file) {
            resolver.print_errors();
            resolver.override_symbols = false;
            resolver.errors = std::move(previous);
            resolver.has_errors = prev_has_errors;
        }
        if (resolver.has_errors) {
            compile_result = false;
            break;
        }

        // compiling the nodes
        gen->current_path = abs_path;
        gen->nodes = std::move(scope.nodes);
        gen->compile_nodes();
        file_nodes.emplace_back(std::move(gen->nodes));

    }

    if (!resolver.errors.empty()) {
        std::cout << std::endl;
        resolver.print_errors();
        std::cout << std::endl;
    }

    if (!gen->errors.empty()) {
        gen->print_errors();
        std::cout << std::endl;
    }

    gen->compile_end();

    if (gen->has_errors) {
        return false;
    }

    return compile_result;

}