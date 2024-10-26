// Copyright (c) Qinetik 2024.

#include "WorkspaceManager.h"
#include "preprocess/ImportGraphMaker.h"
#include "utils/WorkspaceImportGraphImporter.h"
#include "preprocess/ImportGraphVisitor.h"
#include "preprocess/ImportPathHandler.h"
#include "stream/StringInputSource.h"
#include "cst/base/CSTConverter.h"
#include "utils/PathUtils.h"
#include <memory>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <mutex>

std::string resolve_rel_child_path_str(const std::string& root_path, const std::string& file_path);

std::mutex& WorkspaceManager::lex_lock_path_mutex(const std::string& path) {
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

//std::mutex& WorkspaceManager::parse_lock_path_mutex(const std::string& path) {
//    // multiple calls with different paths to this function are allowed
//    // multiple calls with same paths will be processed sequentially
//    parse_file_mutexes_map_mutex.lock();
//    auto lexing = parse_file_mutexes.find(path);
//    // makes a mutex for current path and hold it
//    if(lexing == parse_file_mutexes.end()) parse_file_mutexes[path];
//    auto& mutex = parse_file_mutexes[path];
//    mutex.lock();
//    parse_file_mutexes_map_mutex.unlock();
//    return mutex;
//}

std::shared_ptr<LexResult> WorkspaceManager::get_cached(const std::string& path) {
    auto found = cache.files.find(path);
    if(found != cache.files.end()) {
        return found->second;
    } else {
        return nullptr;
    }
}

std::shared_ptr<ASTResult> WorkspaceManager::get_cached_ast(const std::string& path) {
    auto found = cache.files_ast.find(path);
    if(found != cache.files_ast.end()) {
        return found->second;
    } else {
        return nullptr;
    }
}

bool WorkspaceManager::has_errors(const std::vector<std::shared_ptr<LexResult>>& lexFiles) {
    for(auto& file : lexFiles) {
        if(Diag::has_errors(file->diags)) {
            return true;
        }
    }
    return false;
}

bool WorkspaceManager::has_errors(const std::vector<std::shared_ptr<ASTResult>>& files) {
    for(auto& file : files) {
        if(Diag::has_errors(file->diags)) {
            return true;
        }
    }
    return false;
}

std::shared_ptr<LexResult> WorkspaceManager::get_lexed_no_lock(const std::string& path) {
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
        Lexer lexer(path, reader, &binder);
        lexer.lex();
        result->unit = std::move(lexer.unit);
        result->diags = std::move(lexer.diagnostics);
    } else {
        FileInputSource input_source(path.data());
        if(input_source.has_error()) {
            return nullptr;
        }
        SourceProvider reader(&input_source);
        Lexer lexer(path, reader, &binder);
        lexer.lex();
        result->unit = std::move(lexer.unit);
        result->diags = std::move(lexer.diagnostics);
    }

    cache.files[path] = result;
//    std::cout << "[LSP] Unlocking path mutex " << path << std::endl;
    return result;
}

std::shared_ptr<LexResult> WorkspaceManager::get_lexed(const std::string& path) {
    if(path.empty()) {
        std::cout << "[LSP] Empty path provided to get_lexed function " << std::endl;
        return nullptr;
    }
//    std::cout << "[LSP] Locking path mutex " << path << std::endl;
    auto& mutex = lex_lock_path_mutex(path);
    std::lock_guard guard(mutex, std::adopt_lock_t());
    auto result = get_lexed_no_lock(path);
    return result;
}

std::shared_ptr<ASTResult> WorkspaceManager::get_ast(
        LexResult* lex_result,
        GlobalInterpretScope& comptime_scope
) {
    const auto& path = lex_result->abs_path;
    if(path.empty()) {
        std::cerr << "[LSP] Empty path provided to get_lexed function " << std::endl;
        return nullptr;
    }
    // lock the mutex, so we can check cache
    auto& mutex = lex_lock_path_mutex(path);
    std::lock_guard guard(mutex, std::adopt_lock_t());
    auto found = get_cached_ast(path);
    if(found) {
//        std::cout << "[LSP] AST Cache hit for " << path << std::endl;
        return found;
    } else {
        std::cout << "[LSP] AST Cache miss for " << path << std::endl;
    }
    const auto result_ptr = new ASTResult(
            path,
            ASTUnit(),
            ASTAllocator(nullptr, 0, 0),
            {}
    );
    auto result = std::shared_ptr<ASTResult>(result_ptr);
    auto& allocator = result_ptr->allocator;
    const auto fileId = loc_man.encodeFile(path);
    CSTConverter converter(fileId, is64Bit, comptime_scope, binder, allocator, allocator, allocator);
    converter.convert(lex_result->unit.tokens);
    result->unit = converter.take_unit();
    result->diags = converter.diagnostics;

    cache.files_ast[path] = result;
//    std::cout << "[LSP] Unlocking path mutex " << path << std::endl;
    return result;
}

std::shared_ptr<ASTResult> WorkspaceManager::get_ast(
    const std::string& path,
    GlobalInterpretScope& comptime_scope
) {
    if(path.empty()) {
        std::cout << "[LSP] Empty path provided to get_lexed function " << std::endl;
        return nullptr;
    }
//    std::cout << "[LSP] Locking path mutex " << path << std::endl;
    auto& mutex = lex_lock_path_mutex(path);
    std::lock_guard guard(mutex, std::adopt_lock_t());
//    std::cout << "[LSP] AST Proceeding for path " << path << std::endl;
    auto found = get_cached_ast(path);
    if(found) {
//        std::cout << "[LSP] AST Cache hit for " << path << std::endl;
        return found;
    } else {
        std::cout << "[LSP] AST Cache miss for " << path << std::endl;
    }

    const auto result_ptr = new ASTResult(
            path,
            ASTUnit(),
            ASTAllocator(nullptr, 0, 0),
            {}
    );
    auto result = std::shared_ptr<ASTResult>(result_ptr);
    auto cst = get_lexed_no_lock(path);
    auto& allocator = result_ptr->allocator;
    const auto fileId = loc_man.encodeFile(path);
    CSTConverter converter(fileId, is64Bit, comptime_scope, binder, allocator, allocator, allocator);
    converter.convert(cst->unit.tokens);
    result->unit = converter.take_unit();
    result->diags = converter.diagnostics;

    cache.files_ast[path] = result;
//    std::cout << "[LSP] Unlocking path mutex " << path << std::endl;
    return result;
}

std::string rel_to_lib_system(const std::string &header_path, const std::string& lsp_exe_path) {
    auto system_headers = resolve_sibling(lsp_exe_path, "libs/system");
    if(!std::filesystem::exists(system_headers)) {
        if(!std::filesystem::create_directories(system_headers)) {
            return "";
        }
    }
    return resolve_rel_child_path_str(system_headers, header_path + ".ch");
}

std::shared_ptr<LexResult> WorkspaceManager::get_lexed(const FlatIGFile& flat_file) {
    if(!flat_file.import_path.empty() && flat_file.import_path[0] == '@') {
        if(flat_file.import_path.ends_with(".h") || flat_file.import_path.ends_with(".c")) {
            if(flat_file.import_path.starts_with("@system")) {
                auto header_path = flat_file.import_path.substr(flat_file.import_path.find('/') + 1);
                auto expected_path = rel_to_lib_system(header_path, lsp_exe_path);
                if(expected_path.empty()) {
                    std::cerr << "[LSP] Couldn't resolve header path for " << header_path << std::endl;
                    return nullptr;
                }
//            std::cout << "[LSP] locking path mutex " << flat_file.abs_path << std::endl;
                // locking path mutex so multiple calls with same paths are considered once for translation
                auto& mutex = lex_lock_path_mutex(flat_file.abs_path);
//            std::cout << "[LSP] checking if exists " << flat_file.abs_path << std::endl;
                if(std::filesystem::exists(expected_path)) {
//                std::cerr << "[LSP] System header cache hit " << expected_path << std::endl;
                } else {
                    std::cout << "[LSP] System header cache miss for header " << header_path << " at " << expected_path << " trying " << flat_file.abs_path << std::endl;
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
            } else if(flat_file.import_path.starts_with("@std")) {
                // don't do anything, since
            } else {
                // TODO check path aliases before returning empty
                std::cerr << "[LSP] Doesn't yet support user provided headers / c files" << std::endl;
                return nullptr;
            }
        }
    }
    return get_lexed(flat_file.abs_path);
}

LexImportUnit WorkspaceManager::get_import_unit(const std::string& abs_path, std::atomic<bool>& cancel_flag) {
    // create and return import unit
    LexImportUnit unit;
    if(cancel_flag.load()) {
        return unit;
    }
    // get lex result for the absolute path
    auto result = get_lexed(abs_path);
    if(cancel_flag.load()) {
        return unit;
    }
    // create a function that takes cst tokens in the import graph maker and creates a import graph
    SourceProvider reader(nullptr);
    Lexer lexer(abs_path, reader, &binder);
    ImportGraphVisitor visitor;
    ImportPathHandler handler(compiler_exe_path());
    WorkspaceImportGraphImporter importer(
            &handler,
            &lexer,
            &visitor,
            this
    );
    FlatIGFile flat_file { abs_path };
    unit.ig_root = determine_import_graph_file(&importer, result->unit.tokens, flat_file);
    if(cancel_flag.load()) {
        return unit;
    }
    // flatten the import graph and get lex result for each file
    auto flattened = unit.ig_root.flatten_by_dedupe();
    for(const auto& flat : flattened) {
        if(cancel_flag.load()) {
            return unit;
        }
        auto imported = get_lexed(flat);
        if(imported) {
            unit.files.emplace_back(imported);
        } else {
            unit.ig_root.errors.emplace_back(
                    flat.range,
                    DiagSeverity::Error,
                    std::nullopt,
                    "couldn't open the file '" + flat.abs_path + "'"
            );
        }
    }

    if(cancel_flag.load()) {
        return unit;
    }

    return unit;
}

void WorkspaceManager::get_ast_import_unit(
    std::vector<std::shared_ptr<ASTResult>>& files,
    const LexImportUnit& unit,
    GlobalInterpretScope& comptime_scope,
    std::atomic<bool>& cancel_flag
) {
    for(auto& file : unit.files) {
        if(cancel_flag.load()) break;
        files.emplace_back(get_ast(file.get(), comptime_scope));
    }
}

WorkspaceImportGraphImporter::WorkspaceImportGraphImporter(
        ImportPathHandler* handler,
        Lexer* lexer,
        ImportGraphVisitor* converter,
        WorkspaceManager* manager
) : ImportGraphImporter(handler, lexer, converter), manager(manager) {

}

std::vector<IGFile> WorkspaceImportGraphImporter::process(const std::string &path, const Range& range, IGFile *parent) {
//    auto found = manager->get_cached(path);
//    if(found) {
//        return from_tokens(path, parent, found->tokens);
//    }
    auto overridden_source = manager->get_overridden_source(path);
    if(overridden_source.has_value()){
        StringInputSource input_source(overridden_source.value());
        lexer->provider.switch_source(&input_source);
        lex_source(path, parent->errors);
        return from_tokens(path, parent, lexer->unit.tokens);
    } else {
        return ImportGraphImporter::process(path, range, parent);
    }
}