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

void ASTProcessor::prepare(const std::string& path) {

}

std::vector<FlatIGFile> ASTProcessor::flat_imports(const std::string& path) {

    IGResult result;

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

LabModule* ASTProcessor::get_root_module(LabBuildContext& context, const std::string& path) {
    if(path.ends_with(".lab")) {
        LabBuildCompilerOptions lbc_opts(options->exe_path, options->target_triple, options->is64Bit);
        lab_build(context, path, &lbc_opts);
        return context.root_module;
    } else if(path.ends_with(".ch")) {
        chem::string name("ChemMod");
        chem::string path_str(path.data());
        return context.add_with_type(LabModuleType::RootFile, &name, &path_str, nullptr, 0);
    } else {
        return nullptr;
    }
}

std::vector<FlatIGFile> ASTProcessor::determine_mod_imports(LabModule* module) {
    switch(module->type) {
        case LabModuleType::RootFile:
            return flat_imports(module->path.to_std_string());
        case LabModuleType::Directory:
            throw std::runtime_error("NOT YET IMPLEMENTED DIRECTORY THING");
    }
}

void ASTProcessor::sym_res(Scope& scope, bool is_c_file, const std::string& abs_path) {
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
        Lexer lexer(provider, abs_path, binder.get(), lexer_cbi.get());
        if(options->isCBIEnabled) {
            bind_lexer_cbi(lexer_cbi.get(), provider_cbi.get(), &lexer);
        }

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

        CSTConverter converter(file.abs_path, options->is64Bit, options->target_triple);
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

    if(!out.view().empty()) {
        std::cout << std::endl << out.str() << std::flush;
    }

    return { std::move(scope), true, is_c_file };

}

void ASTProcessor::translate_to_c_no_sym_res(
        ToCAstVisitor& visitor,
        Scope& import_res,
        ShrinkingVisitor& shrinker,
        const FlatIGFile& file
) {
    // translating the nodes
    std::unique_ptr<BenchmarkResults> bm_results;
    if(options->benchmark) {
        bm_results = std::make_unique<BenchmarkResults>();
        bm_results->benchmark_begin();
    }
    visitor.translate(import_res.nodes);
    if(options->benchmark) {
        bm_results->benchmark_end();
        std::cout << std::endl << "[2cTranslation] " << file.abs_path << " Completed " << bm_results->representation() << std::endl;
    }
    if(options->shrink_nodes) {
        shrinker.visit(import_res.nodes);
    }
    file_nodes.emplace_back(std::move(import_res.nodes));
    if(!visitor.errors.empty()) {
        visitor.print_errors(file.abs_path);
        std::cout << std::endl;
    }
    visitor.reset_errors();
}

bool ASTProcessor::translate_to_c(ToCAstVisitor& visitor, ASTImportResult& import_res, ShrinkingVisitor& shrinker, const FlatIGFile& file) {
    // symbol resolution
    sym_res(import_res.scope, import_res.is_c_file, file.abs_path);
    if (resolver->has_errors) {
        return false;
    }
    resolver->reset_errors();
    translate_to_c_no_sym_res(visitor, import_res.scope, shrinker, file);
    return true;
}

void ASTProcessor::end() {

}