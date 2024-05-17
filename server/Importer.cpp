// Copyright (c) Qinetik 2024.

#include "WorkspaceManager.h"
#include "preprocess/ImportGraphMaker.h"
#include "preprocess/CSTSymbolResolver.h"
#include <sstream>
#include <mutex>

std::shared_ptr<LexResult> WorkspaceManager::get_lexed(const std::string& path) {

    // multiple calls with different paths to this function are allowed
    // multiple calls with same paths will be processed sequentially
    lex_file_mutexes_map_mutex.lock();
    auto lexing = lex_file_mutexes.find(path);
    // makes a mutex for current path and hold it
    if(lexing == lex_file_mutexes.end()) lex_file_mutexes[path];
    std::lock_guard<std::mutex> path_lock(lex_file_mutexes[path]);
    lex_file_mutexes_map_mutex.unlock();

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

ImportUnit WorkspaceManager::get_import_unit(const std::string& abs_path, bool publish_diags) {
    // get lex result for the absolute path
    auto result = get_lexed(abs_path);
    // create a function that takes cst tokens in the import graph maker and creates a import graph
    // TODO provide correct executable path
    // TODO cache the import graph
    auto ig = determine_import_graph("", result->tokens, { abs_path });
    // flatten the import graph and get lex result for each file
    auto flattened = ig.root.flatten_by_dedupe();
    // create and return import unit
    ImportUnit unit;
    for(const auto& flat : flattened) {
        unit.files.emplace_back(get_lexed(flat.abs_path));
    }
    CSTSymbolResolver resolver;
    resolver.resolve(&unit);
    if(publish_diags) {
        publish_diagnostics(abs_path, true, { &result->diags, &resolver.diagnostics });
    }
    return unit;
}