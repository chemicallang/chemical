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

std::shared_ptr<LexResult> WorkspaceManager::get_lexed(const std::string& path, bool keep_comments) {
    auto result = std::make_shared<LexResult>();
    if(!get_lexed(result.get(), path, keep_comments)) {
        return nullptr;
    }
    return result;
}

bool WorkspaceManager::get_lexed(LexResult* result, const std::string& path, bool keep_comments) {
    auto overridden_source = get_overridden_source(path);
    result->abs_path = path;
    if (overridden_source.has_value()) {
        StringInputSource input_source(overridden_source.value());
        Lexer lexer(path, &input_source, &binder, result->fileAllocator);
        if(keep_comments) {
            lexer.keep_comments = true;
        }
        lexer.getTokens(result->tokens);
        result->allocator = std::move(lexer.str.allocator);
        result->diags = std::move(lexer.diagnoser.diagnostics);
        result->has_errors = lexer.diagnoser.has_errors;
    } else {
        FileInputSource input_source(path.data());
        if(input_source.has_error()) {
            return false;
        }
        Lexer lexer(path, &input_source, &binder, result->fileAllocator);
        if(keep_comments) {
            lexer.keep_comments = true;
        }
        lexer.getTokens(result->tokens);
        result->allocator = std::move(lexer.str.allocator);
        result->diags = std::move(lexer.diagnoser.diagnostics);
        result->has_errors = lexer.diagnoser.has_errors;
    }
    return true;
}

std::shared_ptr<ASTResult> WorkspaceManager::get_ast_no_lock(Token* start_token, const std::string& path) {

    const auto fileId = loc_man.encodeFile(path);
    const auto result_ptr = new ASTResult(
            path,
            ASTUnit(fileId, chem::string_view(path), nullptr),
            ASTAllocator(10000),
            {}
    );
    auto result = std::shared_ptr<ASTResult>(result_ptr);
    auto& allocator = result_ptr->allocator;

    // TODO scope name and module name aren't done
    const auto modScope = new (allocator.allocate<ModuleScope>()) ModuleScope("", "", nullptr);
    result_ptr->unit.set_parent(modScope);

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

std::shared_ptr<ASTResult> WorkspaceManager::get_decl_ast(const std::string& path) {

    // get the lex import unit
    auto lex_result = get_lexed(path);

    // get the ast unit
    auto ast_unit = get_ast_no_lock(lex_result->tokens.data(), path);

    return ast_unit;

}