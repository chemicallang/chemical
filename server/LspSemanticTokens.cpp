// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/03/2024.
//

#include "LspSemanticTokens.h"
#include "utils/JsonUtils.h"
#include "utils/FileUtils.h"
#include "utils/Utils.h"
#include "LibLsp/lsp/AbsolutePath.h"
#include "LibLsp/lsp/textDocument/publishDiagnostics.h"
#include "stream/SourceProvider.h"
#include "server/analyzers/SemanticTokensAnalyzer.h"

#define DEBUG_TOKENS false
#define OVER_SRC_PRINT false
#define PRINT_TOKENS false

td_semanticTokens_full::response WorkspaceManager::get_semantic_tokens_full(const lsDocumentUri& uri) {
    auto toks = get_semantic_tokens(uri);
    td_semanticTokens_full::response rsp;
    SemanticTokens tokens;
    tokens.data = SemanticTokens::encodeTokens(toks);
    rsp.result = std::move(tokens);
    return std::move(rsp);
}

std::vector<SemanticToken> WorkspaceManager::get_semantic_tokens(const lsDocumentUri& uri) {

    auto path = uri.GetAbsolutePath().path;

    auto overridden_source = get_overridden_source(path);

    std::vector<std::unique_ptr<CSTToken>> lexed;

    std::vector<Diag> errors;

    if (overridden_source.has_value()) {
        if(OVER_SRC_PRINT) std::cout << "[to_semantic_tokens] overridden source : " << overridden_source.value() << '\n';
        std::istringstream iss(overridden_source.value());
        SourceProvider reader(iss);
        Lexer lexer(reader, path);
        lexer.lex();
        lexed = std::move(lexer.tokens);
        errors = std::move(lexer.errors);
    } else {
        if(OVER_SRC_PRINT) std::cout << "[to_semantic_tokens] overridden source not found for : " << path << '\n';
        std::ifstream file;
        file.open(path);
        if (!file.is_open()) {
            std::cerr << "Unknown error opening the file" << '\n';
        }
        SourceProvider reader(file);
        Lexer lexer(reader, path);
        lexer.lex();
        lexed = std::move(lexer.tokens);
        errors = std::move(lexer.errors);
        file.close();
    }

    // publishing diagnostics related to the lexing
    Notify_TextDocumentPublishDiagnostics::notify notify;
    for(const auto &error : errors) {
        notify.params.diagnostics.push_back(lsDiagnostic{
            lsRange(
                    lsPosition(error.range.start.line, error.range.start.character),
                    lsPosition(error.range.end.line, error.range.end.character)
                ),
                lsDiagnosticSeverity::Error,
                std::nullopt,
                std::nullopt,
                std::nullopt,
                error.message
        });
    }

    notify.params.uri = lsDocumentUri::FromPath(uri.GetAbsolutePath());
    std::future<void> futureObj = std::async(std::launch::async, [&]{
        remote->sendNotification(notify);
    });

    // TODO linked tokens
//    if(PRINT_TOKENS) {
//        printTokens(lexed, linker.resolved);
//    }

    if(DEBUG_TOKENS) {
        auto overridden = get_overridden_source(path);
        if(overridden.has_value()) {
            // Writing the source code to a debug file
            writeToProjectFile("debug/source.txt", overridden.value());
            // Writing the source code as ascii to a debug file
            writeAsciiToProjectFile("debug/ascii.txt", overridden.value());
        }
    }

    if(DEBUG_TOKENS) {
        JsonUtils utils;
        utils.serialize("debug/tokens.json", lexed);
    }


    SemanticTokensAnalyzer analyzer;

    analyzer.analyze(lexed);

    return std::move(analyzer.tokens);

}