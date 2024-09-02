// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/03/2024.
//

#include "utils/FileUtils.h"
#include "LibLsp/lsp/AbsolutePath.h"
#include "LibLsp/lsp/textDocument/publishDiagnostics.h"
#include "server/analyzers/SemanticTokensAnalyzer.h"
#include "WorkspaceManager.h"
#include "LibLsp/JsonRpc/RemoteEndPoint.h"
#include "compiler/SymbolResolver.h"
#include <future>

#define DEBUG_TOKENS false
#define PRINT_TOKENS false

td_semanticTokens_full::response WorkspaceManager::get_semantic_tokens_full(const lsDocumentUri& uri) {
    auto abs_path = canonical(uri.GetAbsolutePath().path);
    auto toks = get_semantic_tokens(abs_path);
    // when user requests semantic tokens, we also trigger publish diagnostics for the file
    publish_diagnostics_complete_async(abs_path);
    // sending tokens
    td_semanticTokens_full::response rsp;
    SemanticTokens tokens;
    tokens.data = SemanticTokens::encodeTokens(toks);
    rsp.result = std::move(tokens);
    return std::move(rsp);
}

void WorkspaceManager::publish_diagnostics(const std::string& path, bool async, const std::vector<std::vector<Diag>*>& diags) {
    Notify_TextDocumentPublishDiagnostics::notify notify;
    for(auto diag : diags) {
        for(const auto &error : *diag) {
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
    }
    notify.params.uri = lsDocumentUri::FromPath(AbsolutePath(path));
    if(async) {
        std::future<void> futureObj = std::async(std::launch::async, [&] {
            remote->sendNotification(notify);
        });
    } else {
        remote->sendNotification(notify);
    }
}

void WorkspaceManager::publish_diagnostics_complete(const std::string& path) {

    std::vector<std::vector<Diag>*> diag_ptrs;
    bool async = false;

    // get the lex import unit
    auto import_unit = get_import_unit(path);
    auto& last_lex_result = import_unit.files[import_unit.files.size() - 1];

    // check publish diagnostics hasn't been cancelled
    if(publish_diagnostics_cancel_flag.load()) {
        return;
    }

    // put lex diagnostics in the diag pointers
    diag_ptrs.emplace_back(&last_lex_result->diags);

    // since lexing process generated errors, no need to parse
    if(has_errors(import_unit)) {
        publish_diagnostics(path, async, diag_ptrs);
        return;
    }

    // get the ast import unit
    auto ast_import_unit = get_ast_import_unit(import_unit);
    auto& last_ast_result = ast_import_unit.files[ast_import_unit.files.size() - 1];

    // check publish diagnostics hasn't been cancelled
    if(publish_diagnostics_cancel_flag.load()) {
        return;
    }

    // put ast conversion diagnostics
    diag_ptrs.emplace_back(&last_ast_result->diags);

    // since parsing process generated errors, no need to symbol resolve
    if(has_errors(import_unit)) {
        publish_diagnostics(path, async, diag_ptrs);
        return;
    }

    // let's do symbol resolution
    SymbolResolver resolver(is64Bit);

    // doing all but last file
    unsigned i = 0;
    const auto last = ast_import_unit.files.size() - 1;
    while(i < last) {
        // check publish diagnostics hasn't been cancelled
        if(publish_diagnostics_cancel_flag.load()) {
            return;
        }
        auto& file = ast_import_unit.files[i];
        resolver.resolve_file(file->unit.scope, file->abs_path);
        resolver.diagnostics.clear();
        i++;
    }

    // check publish diagnostics hasn't been cancelled
    if(publish_diagnostics_cancel_flag.load()) {
        return;
    }

    // doing last file
    auto& last_file = ast_import_unit.files[last];
    resolver.resolve_file(last_file->unit.scope, last_file->abs_path);
    diag_ptrs.emplace_back(&resolver.diagnostics);

    // publish all the diagnostics
    publish_diagnostics(path, async, diag_ptrs);

}

static constexpr bool DEBUGGING_PUBLISH_DIAGNOSTICS = false;

void WorkspaceManager::publish_diagnostics_complete_async(std::string path) {

    std::lock_guard<std::mutex> lock(publish_diagnostics_mutex);

    // Signal the current task to cancel if it's running
    if (publish_diagnostics_task.valid()) {
        publish_diagnostics_cancel_flag.store(true);
        publish_diagnostics_task.wait();  // Ensure the previous task has completed before launching a new one
    }

    // Reset the cancel flag for the new task
    publish_diagnostics_cancel_flag.store(false);

    if(DEBUGGING_PUBLISH_DIAGNOSTICS) {
        // DEBUGGING so launching it synchronously so exceptions are reported nicely
        publish_diagnostics_complete(path);
    } else {
        publish_diagnostics_task = std::async(std::launch::async, [path, this] {
            publish_diagnostics_complete(path);
        });
    }

}

std::vector<SemanticToken> WorkspaceManager::get_semantic_tokens(const std::string& abs_path) {

    auto file = get_lexed(abs_path);

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
    analyzer.analyze(file->unit.tokens);
    return std::move(analyzer.tokens);

}