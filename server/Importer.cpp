// Copyright (c) Chemical Language Foundation 2025.

#include "WorkspaceManager.h"
#include "preprocess/ImportPathHandler.h"
#include "stream/StringInputSource.h"
#include "utils/PathUtils.h"
#include "stream/SourceProvider.h"
#include "parser/Parser.h"
#include "stream/FileInputSource.h"
#include "ast/structures/ModuleScope.h"
#include "lexer/Lexer.h"
#include <memory>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <mutex>

std::string resolve_rel_child_path_str(const std::string& root_path, const std::string& file_path);

std::mutex& WorkspaceManager::parse_lock_path_mutex(const std::string& path) {
    // multiple calls with different paths to this function are allowed
    // multiple calls with same paths will be processed sequentially
    lex_file_mutexes_map_mutex.lock();
    auto lexing = parse_file_mutexes.find(path);
    // makes a mutex for current path and hold it
    if(lexing == parse_file_mutexes.end()) parse_file_mutexes[path];
    auto& mutex = parse_file_mutexes[path];
    mutex.lock();
    lex_file_mutexes_map_mutex.unlock();
    return mutex;
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

std::shared_ptr<LexResult> WorkspaceManager::get_lexed(const std::string& path) {
    auto overridden_source = get_overridden_source(path);
    auto result = std::make_shared<LexResult>();
    result->abs_path = path;
    if (overridden_source.has_value()) {
        StringInputSource input_source(overridden_source.value());
        SourceProvider reader(&input_source);
        Lexer lexer(path, &input_source, &binder, result->allocator);
        lexer.getTokens(result->tokens);
        result->diags = std::move(lexer.diagnoser.diagnostics);
    } else {
        FileInputSource input_source(path.data());
        if(input_source.has_error()) {
            return nullptr;
        }
        SourceProvider reader(&input_source);
        Lexer lexer(path, &input_source, &binder, result->allocator);
        lexer.getTokens(result->tokens);
        result->diags = std::move(lexer.diagnoser.diagnostics);
    }
    return result;
}

std::shared_ptr<ASTResult> WorkspaceManager::get_ast_no_lock(
        Token* start_token,
        const std::string& path,
        GlobalInterpretScope& comptime_scope
) {
    // TODO memory leak, make this module scope free
    const auto modScope = new ModuleScope("", "", nullptr);

    const auto fileId = loc_man.encodeFile(path);
    const auto result_ptr = new ASTResult(
            path,
            ASTUnit(fileId, chem::string_view(path), modScope),
            ASTAllocator(0),
            {}
    );
    auto result = std::shared_ptr<ASTResult>(result_ptr);
    auto& allocator = result_ptr->allocator;

    // creating a parser and parsing the file
    Parser parser(
            fileId,
            std::string_view(path),
            start_token,
            loc_man,
            allocator,
            allocator,
            typeBuilder,
            is64Bit,
            &binder
    );
    parser.parse(result->unit.scope.body.nodes);

    result->diags = std::move(parser.diagnostics);

//    std::cout << "[LSP] Unlocking path mutex " << path << std::endl;
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
    auto& mutex = parse_lock_path_mutex(path);
    std::lock_guard guard(mutex, std::adopt_lock_t());
    auto found = get_cached_ast(path);
    if(found) {
//        std::cout << "[LSP] AST Cache hit for " << path << std::endl;
        return found;
    } else {
        std::cout << "[LSP] AST Cache miss for " << path << std::endl;
    }

    auto result = get_ast_no_lock(const_cast<Token*>(lex_result->tokens.data()), path, comptime_scope);

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
    auto& mutex = parse_lock_path_mutex(path);
    std::lock_guard guard(mutex, std::adopt_lock_t());
//    std::cout << "[LSP] AST Proceeding for path " << path << std::endl;
    auto found = get_cached_ast(path);
    if(found) {
//        std::cout << "[LSP] AST Cache hit for " << path << std::endl;
        return found;
    } else {
        std::cout << "[LSP] AST Cache miss for " << path << std::endl;
    }

    auto cst = get_lexed(path);

    auto result = get_ast_no_lock(const_cast<Token*>(cst->tokens.data()), path, comptime_scope);

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
                auto& mutex = parse_lock_path_mutex(flat_file.abs_path);
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

    unit.files.emplace_back(result);

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