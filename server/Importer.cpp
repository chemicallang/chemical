// Copyright (c) Chemical Language Foundation 2025.

#include "WorkspaceManager.h"
#include "preprocess/ImportPathHandler.h"
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
        InputSource input_source(overridden_source.value().c_str(), overridden_source.value().size());
        Lexer lexer(path, input_source, &binder, result->fileAllocator);
        if(keep_comments) {
            lexer.keep_comments = true;
        }
        lexer.getTokens(result->tokens);
        result->diags = std::move(lexer.diagnoser.diagnostics);
        result->has_errors = lexer.diagnoser.has_errors;
    } else {
        FileInputSource input_source(path.data());
        if(input_source.error()) {
            return false;
        }
        Lexer lexer(path, input_source, &binder, result->fileAllocator);
        if(keep_comments) {
            lexer.keep_comments = true;
        }
        lexer.getTokens(result->tokens);
        result->diags = std::move(lexer.diagnoser.diagnostics);
        result->has_errors = lexer.diagnoser.has_errors;
    }
    return true;
}