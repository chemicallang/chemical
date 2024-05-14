// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/03/2024.
//

#include "LspSemanticTokens.h"
#include "utils/JsonUtils.h"
#include "utils/FileUtils.h"
#include "LibLsp/lsp/AbsolutePath.h"
#include "LibLsp/lsp/textDocument/publishDiagnostics.h"
#include "stream/SourceProvider.h"
#include "server/analyzers/SemanticTokensAnalyzer.h"

#define DEBUG_TOKENS false
#define PRINT_TOKENS false

td_semanticTokens_full::response WorkspaceManager::get_semantic_tokens_full(const lsDocumentUri& uri) {
    auto toks = get_semantic_tokens(uri);
    td_semanticTokens_full::response rsp;
    SemanticTokens tokens;
    tokens.data = SemanticTokens::encodeTokens(toks);
    rsp.result = std::move(tokens);
    return std::move(rsp);
}
void WorkspaceManager::publish_diagnostics(const std::string& path, std::vector<Diag>& diags, bool async) {
    Notify_TextDocumentPublishDiagnostics::notify notify;
    for(const auto &error : diags) {
        notify.params.diagnostics.emplace_back(
                lsRange(
                        lsPosition(error.range.start.line, error.range.start.character),
                        lsPosition(error.range.end.line, error.range.end.character)
                ),
                (lsDiagnosticSeverity) (error.severity.value()),
                std::nullopt,
                std::nullopt,
                std::nullopt,
                error.message
        );
    }
    notify.params.uri = lsDocumentUri::FromPath(AbsolutePath(path));
    if(async) {
        std::future<void> futureObj = std::async(std::launch::async, [&] {
            remote->sendNotification(notify);
        });
    }
}

LexResult WorkspaceManager::get_lexed(const std::string& path) {
    auto overridden_source = get_overridden_source(path);
    LexResult result;
    if (overridden_source.has_value()) {
        std::istringstream iss(overridden_source.value());
        SourceProvider reader(iss);
        Lexer lexer(reader, path);
        lexer.lex();
        result.tokens = std::move(lexer.tokens);
        result.diags = std::move(lexer.errors);
    } else {
        std::ifstream file;
        file.open(path);
        if (!file.is_open()) {
            std::cerr << "Unknown error opening the file" << '\n';
        }
        SourceProvider reader(file);
        Lexer lexer(reader, path);
        lexer.lex();
        result.tokens = std::move(lexer.tokens);
        result.diags = std::move(lexer.errors);
        file.close();
    }
    return result;
}

std::vector<SemanticToken> WorkspaceManager::get_semantic_tokens(const lsDocumentUri& uri) {

    auto path = uri.GetAbsolutePath().path;
    auto result = get_lexed(path);
    // publishing diagnostics gathered during lexing
    publish_diagnostics(path, result.diags, true);

#if defined PRINT_TOKENS && PRINT_TOKENS
    printTokens(lexed, linker.resolved);
#endif

#if defined DEBUG_TOKENS && DEBUG_TOKENS
    auto overridden = get_overridden_source(path);
    if(overridden.has_value()) {
        // Writing the source code to a debug file
        writeToProjectFile("debug/source.txt", overridden.value());
        // Writing the source code as ascii to a debug file
        writeAsciiToProjectFile("debug/ascii.txt", overridden.value());
    }
    // serializing tokens to tokens json file
    JsonUtils utils;
    utils.serialize("debug/tokens.json", result.tokens);
#endif

    SemanticTokensAnalyzer analyzer;
    analyzer.analyze(result.tokens);
    return std::move(analyzer.tokens);

}