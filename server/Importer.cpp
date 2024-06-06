// Copyright (c) Qinetik 2024.

#include "WorkspaceManager.h"
#include "preprocess/ImportGraphMaker.h"
#include "preprocess/CSTSymbolResolver.h"
#include "utils/WorkspaceImportGraphImporter.h"
#include "preprocess/ImportGraphVisitor.h"
#include "preprocess/ImportPathHandler.h"
#include <sstream>
#include <mutex>

std::shared_ptr<LexResult> WorkspaceManager::get_cached(const std::string& path) {
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
    } else {
        return nullptr;
    }
}

std::shared_ptr<LexResult> WorkspaceManager::get_lexed(const std::string& path) {

    auto found = get_cached(path);
    if(found) {
//        std::cout << "[LSP] Cache hit for " << path << std::endl;
        return found;
    } else {
//        std::cout << "[LSP] Cache miss for " << path << std::endl;
    }

    auto overridden_source = get_overridden_source(path);
    auto result = std::make_shared<LexResult>();
    result->abs_path = path;
    if (overridden_source.has_value()) {
        std::istringstream iss(overridden_source.value());
        SourceProvider reader(&iss);
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
        SourceProvider reader(&file);
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
    std::ifstream file;
    SourceProvider reader(&file);
    Lexer lexer(reader, abs_path);
    ImportGraphVisitor visitor;
    ImportPathHandler handler("");
    WorkspaceImportGraphImporter importer(
            &handler,
            &lexer,
            &visitor,
            this
    );
    FlatIGFile flat_file { abs_path };
    auto ig = determine_import_graph(&importer, result->tokens, flat_file);
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
        publish_diagnostics(abs_path, true, { &ig.root.errors, &result->diags, &resolver.diagnostics });
    }
    return unit;
}

WorkspaceImportGraphImporter::WorkspaceImportGraphImporter(
        ImportPathHandler* handler,
        Lexer* lexer,
        ImportGraphVisitor* converter,
        WorkspaceManager* manager
) : ImportGraphImporter(handler, lexer, converter), manager(manager) {

}

std::vector<IGFile> WorkspaceImportGraphImporter::process(const std::string &path, IGFile *parent) {
//    auto found = manager->get_cached(path);
//    if(found) {
//        return from_tokens(path, parent, found->tokens);
//    }
    auto overridden_source = manager->get_overridden_source(path);
    if(overridden_source.has_value()){
        std::istringstream iss(overridden_source.value());
        lexer->provider.stream = &iss;
        lex_source(path, parent->errors);
        return from_tokens(path, parent, lexer->tokens);
    } else {
        std::ifstream file_stream;
        lexer->provider.stream = &file_stream;
        return ImportGraphImporter::process(path, parent);
    }
}