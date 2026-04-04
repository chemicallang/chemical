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
    auto fileId = loc_man.encodeFile(path);
    auto result = std::make_shared<LexResult>(fileId);
    if(!get_lexed(result.get(), path, keep_comments)) {
        return nullptr;
    }
    return result;
}

bool WorkspaceManager::get_lexed(LexResult* result, const std::string& path, bool keep_comments) {
    auto overridden_source_opt = get_overridden_source(path);
    result->abs_path = path;
    if (overridden_source_opt.has_value()) {
        result->overridden_source = std::move(overridden_source_opt.value());
        auto& overridden_source = result->overridden_source;
        InputSource input_source(overridden_source.c_str(), overridden_source.size());
        Lexer lexer(path, input_source, &binder, result->fileAllocator);
        if(keep_comments) {
            lexer.keep_comments = true;
        }
        lexer.getTokens(result->tokens);
        result->diags = std::move(lexer.diagnoser.diagnostics);
        result->has_errors = lexer.diagnoser.has_errors();
    } else {
        auto& input_source = result->fileSource;
        input_source.open(path);
        if(input_source.error()) {
            return false;
        }
        Lexer lexer(path, input_source, &binder, result->fileAllocator);
        if(keep_comments) {
            lexer.keep_comments = true;
        }
        lexer.getTokens(result->tokens);
        result->diags = std::move(lexer.diagnoser.diagnostics);
        result->has_errors = lexer.diagnoser.has_errors();
    }
    return true;
}