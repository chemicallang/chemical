// Copyright (c) Qinetik 2024.

#include "WorkspaceManager.h"
#include "preprocess/ImportGraphMaker.h"
#include "preprocess/CSTSymbolResolver.h"
#include "utils/WorkspaceImportGraphImporter.h"
#include "preprocess/ImportGraphVisitor.h"
#include "preprocess/ImportPathHandler.h"
#include "stream/StringInputSource.h"
#include <sstream>
#include <filesystem>
#include <mutex>

std::string resolve_rel_child_path_str(const std::string& root_path, const std::string& file_path);

std::mutex& WorkspaceManager::lock_path_mutex(const std::string& path) {
    // multiple calls with different paths to this function are allowed
    // multiple calls with same paths will be processed sequentially
    lex_file_mutexes_map_mutex.lock();
    auto lexing = lex_file_mutexes.find(path);
    // makes a mutex for current path and hold it
    if(lexing == lex_file_mutexes.end()) lex_file_mutexes[path];
    auto& mutex = lex_file_mutexes[path];
    mutex.lock();
    lex_file_mutexes_map_mutex.unlock();
    return mutex;
}

std::shared_ptr<LexResult> WorkspaceManager::get_cached(const std::string& path) {
    auto found = cache.files.find(path);
    if(found != cache.files.end()) {
        return found->second;
    } else {
        return nullptr;
    }
}

std::shared_ptr<LexResult> WorkspaceManager::get_lexed(const std::string& path) {
    if(path.empty()) {
        std::cout << "[LSP] Empty path provided to get_lexed function " << std::endl;
        return nullptr;
    }
//    std::cout << "[LSP] Locking path mutex " << path << std::endl;
    auto& mutex = lock_path_mutex(path);
    std::lock_guard guard(mutex, std::adopt_lock_t());
//    std::cout << "[LSP] Proceeding for path " << path << std::endl;
    auto found = get_cached(path);
    if(found) {
//        std::cout << "[LSP] Cache hit for " << path << std::endl;
        return found;
    } else {
        std::cout << "[LSP] Cache miss for " << path << std::endl;
    }
    auto overridden_source = get_overridden_source(path);
    auto result = std::make_shared<LexResult>();
    result->abs_path = path;
    if (overridden_source.has_value()) {
        StringInputSource input_source(overridden_source.value());
        SourceProvider reader(&input_source);
        Lexer lexer(reader);
        lexer.lex();
        result->tokens = std::move(lexer.tokens);
        result->diags = std::move(lexer.diagnostics);
    } else {
        FileInputSource input_source(path);
        if(input_source.has_error()) {
            result->diags.emplace_back(
                    Range {0,0,0,0},
                    DiagSeverity::Error,
                    path,
                    "couldn't open the file"
            );
            return result;
        }
        SourceProvider reader(&input_source);
        Lexer lexer(reader);
        lexer.lex();
        result->tokens = std::move(lexer.tokens);
        result->diags = std::move(lexer.diagnostics);
    }

    cache.files[path] = result;
//    std::cout << "[LSP] Unlocking path mutex " << path << std::endl;
    return result;
}

std::string rel_to_lib_system(const std::string &header_path, const std::string& lsp_exe_path) {
    auto system_headers = resolve_rel_parent_path_str(lsp_exe_path, "lib/system");
    if(system_headers.empty()) {
        std::cerr << "[LSP] Couldn't resolve lib/system directory path relative to LSP executable" << std::endl;
        return "";
    }
    return resolve_rel_child_path_str(system_headers, header_path + ".ch");
}

std::shared_ptr<LexResult> WorkspaceManager::get_lexed(const FlatIGFile& flat_file) {
    if(flat_file.import_path.ends_with(".h") || flat_file.import_path.ends_with(".c")) {
        if(flat_file.import_path.starts_with("@system")) {
            auto header_path = flat_file.import_path.substr(flat_file.import_path.find('/') + 1);
            auto expected_path = rel_to_lib_system(header_path, lsp_exe_path);
            if(expected_path.empty()) {
                 std::cerr << "[LSP] Couldn't resolve header path for " << header_path << std::endl;
                goto empty_return;
            }
//            std::cout << "[LSP] locking path mutex " << flat_file.abs_path << std::endl;
            // locking path mutex so multiple calls with same paths are considered once for translation
            auto& mutex = lock_path_mutex(flat_file.abs_path);
//            std::cout << "[LSP] checking if exists " << flat_file.abs_path << std::endl;
            if(std::filesystem::exists(expected_path)) {
//                std::cerr << "[LSP] System header cache hit " << expected_path << std::endl;
            } else {
                std::cout << "[LSP] System header cache miss for header " << header_path << " at " << expected_path << std::endl;
                auto result = get_c_translated(flat_file.abs_path, expected_path);
                if(result.second == -1) {
                    std::cerr << "[LSP] status code 1 when translating c header " << header_path << " at " << expected_path << std::endl;
                } else {
                    std::cout << "[LSP] Translation C output " << std::endl << result.first << std::endl;
                }
            }
//            std::cout << "[LSP] Unlocking path mutex " << flat_file.abs_path << std::endl;
            mutex.unlock();
            return get_lexed(expected_path);
        } else {
            std::cerr << "[LSP] Doesn't yet support user provided headers / c files" << std::endl;
            goto empty_return;
        }
    }
    return get_lexed(flat_file.abs_path);
    empty_return: {
        return std::make_shared<LexResult>();
    };
}

ImportUnit WorkspaceManager::get_import_unit(const std::string& abs_path, bool publish_diags) {
    // get lex result for the absolute path
    auto result = get_lexed(abs_path);
    // create a function that takes cst tokens in the import graph maker and creates a import graph
    SourceProvider reader(nullptr);
    Lexer lexer(reader);
    ImportGraphVisitor visitor;
    ImportPathHandler handler(compiler_exe_path());
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
        unit.files.emplace_back(get_lexed(flat));
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
        StringInputSource input_source(overridden_source.value());
        lexer->provider.switch_source(&input_source);
        lex_source(path, parent->errors);
        return from_tokens(path, parent, lexer->tokens);
    } else {
        return ImportGraphImporter::process(path, parent);
    }
}