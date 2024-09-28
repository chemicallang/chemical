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
    // first we collect all the diagnostics, this will symbol resolve everything till the last file
    publish_diagnostics_complete_async(abs_path, std::launch::deferred, true, true);
    // the file we are collecting tokens for, has been symbol resolved by publish diagnostics
    // so now tokens contain references to the ast anys
    auto toks = get_semantic_tokens(abs_path);
    // sending tokens
    td_semanticTokens_full::response rsp;
    SemanticTokens tokens;
    tokens.data = SemanticTokens::encodeTokens(toks);
    rsp.result = std::move(tokens);
    return std::move(rsp);
}

void build_notify_request(
        Notify_TextDocumentPublishDiagnostics::notify& notify,
        const std::string& path,
        const std::vector<std::vector<Diag>*>& diags
) {
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
}

void WorkspaceManager::notify_diagnostics_async(
    const std::string& path,
    const std::vector<std::vector<Diag>*>& diags
) {
    const auto notify = new Notify_TextDocumentPublishDiagnostics::notify;
    build_notify_request(*notify, path, diags);
    std::future<void> futureObj = std::async(std::launch::async, [this, notify] {
        remote->sendNotification(*notify);
        delete notify;
    });
}

void WorkspaceManager::notify_diagnostics(
    const std::string& path,
    const std::vector<std::vector<Diag>*>& diags
) {
    Notify_TextDocumentPublishDiagnostics::notify notify;
    build_notify_request(notify, path, diags);
    remote->sendNotification(notify);
}

std::vector<Diag> WorkspaceManager::sym_res_import_unit(ASTImportUnit& ast_import_unit, std::atomic<bool>& cancel_flag) {

    const unsigned int resolver_mem_size = 10000; // pre allocated 10kb on the stack
    char resolver_memory[resolver_mem_size];
    // all heap allocations will not be batched
    ASTAllocator resolver_allocator(resolver_memory, resolver_mem_size, 0);

    // let's do symbol resolution
    SymbolResolver resolver(
            ast_import_unit.comptime_scope,
            is64Bit,
            resolver_allocator,
            nullptr,
            nullptr
    );
    // doing all but last file
    unsigned i = 0;
    const auto last = ast_import_unit.files.size() - 1;
    while(i < last) {
        // check publish diagnostics hasn't been cancelled
        if(cancel_flag.load()) {
            return {};
        }
        auto& file = ast_import_unit.files[i];
        resolver.mod_allocator = &file->allocator;
        resolver.ast_allocator = &file->allocator;
        resolver.resolve_file(file->unit.scope, file->abs_path);
        resolver.diagnostics.clear();
        i++;
    }

    // check publish diagnostics hasn't been cancelled
    if(cancel_flag.load()) {
        return {};
    }

    // doing last file
    auto& last_file = ast_import_unit.files[last];
    resolver.resolve_file(last_file->unit.scope, last_file->abs_path);

    return std::move(resolver.diagnostics);

}

void WorkspaceManager::publish_diagnostics_complete(
    const std::string& path,
    bool notify_async,
    std::atomic<bool>& cancel_flag
) {

    std::vector<std::vector<Diag>*> diag_ptrs;

    // get the lex import unit
    auto import_unit = get_import_unit(path, cancel_flag);

    // check publish diagnostics hasn't been cancelled
    if(cancel_flag.load()) {
        return;
    }

    auto& last_lex_result = import_unit.files[import_unit.files.size() - 1];

    // put lex diagnostics in the diag pointers
    diag_ptrs.emplace_back(&last_lex_result->diags);

    // since lexing process generated errors, no need to parse
    if(has_errors(import_unit)) {
        notify_diagnostics(path, diag_ptrs);
        return;
    }

    // get the ast import unit
    auto ast_import_unit = get_ast_import_unit(import_unit, cancel_flag);
    // check publish diagnostics hasn't been cancelled
    if(cancel_flag.load()) {
        return;
    }

    auto& last_ast_result = ast_import_unit.files[ast_import_unit.files.size() - 1];

    // put ast conversion diagnostics
    diag_ptrs.emplace_back(&last_ast_result->diags);

    // since parsing process generated errors, no need to symbol resolve
    if(has_errors(import_unit)) {
        notify_diagnostics(path, diag_ptrs);
        return;
    }

    // symbol resolve the import unit, get the last file's diagnostics
    auto res_diags = sym_res_import_unit(ast_import_unit, cancel_flag);
    diag_ptrs.emplace_back(&res_diags);

    // publish all the diagnostics
    if(notify_async) {
        notify_diagnostics_async(path, diag_ptrs);
    } else {
        notify_diagnostics(path, diag_ptrs);
    }

}

template<typename TaskLambda>
void WorkspaceManager::queued_single_invocation(
        std::mutex& task_mutex,
        std::future<void>& task,
        std::atomic<bool>& cancel_flag,
        const TaskLambda& lambda
) {

    std::lock_guard<std::mutex> lock(task_mutex);

    // Signal the current task to cancel if it's running
    if (task.valid()) {
        cancel_flag.store(true);
        task.wait();  // Ensure the previous task has completed before launching a new one
    }

    // Reset the cancel flag for the new task
    cancel_flag.store(false);

    task = std::async(std::launch::async, lambda);

}

void WorkspaceManager::publish_diagnostics_complete_async(
    const std::string& path,
    std::launch launch_policy,
    bool notify_async,
    bool do_synchronous
) {

    std::lock_guard<std::mutex> lock(publish_diagnostics_mutex);

    // Signal the current task to cancel if it's running
    if (publish_diagnostics_task.valid()) {
        publish_diagnostics_cancel_flag.store(true);
        publish_diagnostics_task.wait();  // Ensure the previous task has completed before launching a new one
    }

    // Reset the cancel flag for the new task
    publish_diagnostics_cancel_flag.store(false);

    publish_diagnostics_task = std::async(launch_policy, [path, this, notify_async] {
        publish_diagnostics_complete(path, notify_async, publish_diagnostics_cancel_flag);
    });

    if(do_synchronous) {
        publish_diagnostics_task.wait();
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