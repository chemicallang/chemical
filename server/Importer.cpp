// Copyright (c) Qinetik 2024.

#include "WorkspaceManager.h"
#include "preprocess/ImportGraphMaker.h"
#include <sstream>

std::shared_ptr<LexResult> WorkspaceManager::get_lexed(const std::string& path) {
    auto found = cache.files.find(path);
    if(found != cache.files.end()) {
        return found->second;
    }
    auto overridden_source = get_overridden_source(path);
    auto result = std::make_shared<LexResult>();
    if (overridden_source.has_value()) {
        std::istringstream iss(overridden_source.value());
        SourceProvider reader(iss);
        Lexer lexer(reader, path);
        lexer.lex();
        result->tokens = std::move(lexer.tokens);
        result->diags = std::move(lexer.errors);
    } else {
        std::ifstream file;
        file.open(path);
        if (!file.is_open()) {
            result->diags.emplace_back(
                Range {0,0,0,0},
                DiagSeverity::Error,
                path,
                "couldn't open the file"
            );
            return result;
        }
        SourceProvider reader(file);
        Lexer lexer(reader, path);
        lexer.lex();
        result->tokens = std::move(lexer.tokens);
        result->diags = std::move(lexer.errors);
        file.close();
    }

    cache.files[path] = result;
    return result;
}

ImportUnit WorkspaceManager::get_import_unit(const std::string& abs_path) {
    // get lex result for the absolute path
    auto result = get_lexed(abs_path);
    // create a function that takes cst tokens in the import graph maker and creates a import graph
    // TODO provide correct executable path
    // TODO cache the import graph
    auto ig = determine_import_graph("", result->tokens, { abs_path });
    // return if contains errors
    if(!ig.errors.empty()) {
        ImportUnit unit;
        unit.errors = std::move(ig.errors);
//        for(auto& err : unit.errors) {
//            std::cout << "[ErrorUnit]" << err.ansi_representation(err.doc_url.value()) << std::endl;
//        }
        return unit;
    }
    // flatten the import graph and get lex result for each file
    auto flattened = ig.root.flatten_by_dedupe();
    // create and return import unit
    ImportUnit unit;
    for(const auto& flat : flattened) {
        unit.files.emplace_back(get_lexed(flat.abs_path));
    }
    return unit;
}