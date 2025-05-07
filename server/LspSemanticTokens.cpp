// Copyright (c) Chemical Language Foundation 2025.

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

/**
 * why are diagnostics published here ?
 * because when publishing diagnostics for a file, we have to symbol resolve it completely
 * all it's imported files are symbol resolved too, the files imported don't change so they are cached
 * when a file is modified, we lex the file, convert it to ast, symbol resolver the entire tree (will be improved)
 * and return the tokens to the IDE, not the ideal
 * when semantic tokens are provided, they must know which symbols correspond to which nodes (functions / struct)
 * to provide better highlighting, when all the tokens are linked (done at symbol resolution), we can do that
 * when doing symbol resolution, we also collect diagnostics, which is tightly coupled with symbol resolution
 * so we do everything here, that's why notify_async is true, sending notification is done asynchronously
 */
td_semanticTokens_full::response WorkspaceManager::get_semantic_tokens_full(const lsDocumentUri& uri) {
    auto abs_path = canonical(uri.GetAbsolutePath().path);
    // get the import unit, while publishing diagnostics asynchronously
    // publish diagnostics will return ast import unit ref
    publish_diagnostics(abs_path);
    // tokens for the last file
    auto last_file = get_lexed(abs_path);
    auto toks = get_semantic_tokens(*last_file);
    // preparing response
    SemanticTokens tokens;
    tokens.data = SemanticTokens::encodeTokens(toks);
    td_semanticTokens_full::response rsp;
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

void WorkspaceManager::notify_diagnostics_sync(
    const std::string& path,
    const std::vector<std::vector<Diag>*>& diags
) {
    Notify_TextDocumentPublishDiagnostics::notify notify;
    build_notify_request(notify, path, diags);
    remote->sendNotification(notify);
}

std::vector<Diag> WorkspaceManager::sym_res_import_unit(
    std::vector<std::shared_ptr<ASTResult>>& ast_files,
    GlobalInterpretScope& comptime_scope,
    std::atomic<bool>& cancel_flag
) {

    const unsigned int resolver_mem_size = 10000; // pre allocated 10kb on the stack
    // all heap allocations will not be batched
    ASTAllocator resolver_allocator(resolver_mem_size);

    // let's do symbol resolution
    // TODO nullptr being passed as allocator
    SymbolResolver resolver(
            comptime_scope,
            pathHandler,
            is64Bit,
            resolver_allocator,
            nullptr,
            nullptr
    );

    // when preparing compiler functions, some functions may use module or ast level allocator
    resolver.mod_allocator = &resolver_allocator;
    resolver.ast_allocator = &resolver_allocator;
    // prepare top level compiler functions
    if(global_container) {
        comptime_scope.rebind_container(resolver, global_container);
    } else {
        global_container = comptime_scope.create_container(resolver);
    }

    // doing all but last file
    unsigned i = 0;
    const auto last = ast_files.size() - 1;
    while(i < last) {
        // check it hasn't been cancelled
        if(cancel_flag.load()) {
            return {};
        }
        auto& file = ast_files[i];
        resolver.mod_allocator = &file->allocator;
        resolver.ast_allocator = &file->allocator;
        // TODO resolve here
//        resolver.resolve_file(file->unit.scope, file->abs_path);
        resolver.diagnostics.clear();
        i++;
    }

    // check it hasn't been cancelled
    if(cancel_flag.load()) {
        return {};
    }

    // doing last file
    auto& last_file = ast_files[last];
    // TODO resolve here
//    resolver.resolve_file(last_file->unit.scope, last_file->abs_path);

    return std::move(resolver.diagnostics);

}

ASTImportUnitRef WorkspaceManager::get_ast_import_unit(
    const std::string& path,
    std::atomic<bool>& cancel_flag
) {

    // lock for single invocation
    std::lock_guard lock(ast_import_unit_mutex);

    // get the module for the given file
    const auto mod = get_mod_of(chem::string_view(path));

    // check the cache
    auto found = cache.cached_units.find(path);
    if(found != cache.cached_units.end()) {
        auto cachedUnitPtr = found->second;
        auto& cachedUnit = *cachedUnitPtr;
        // check that every file in cached import unit is valid
        auto lex_result = cachedUnit.lex_result.lock();
        auto ast_result = cachedUnit.ast_result.lock();
        if(lex_result && ast_result) {
            return ASTImportUnitRef(true, path, mod, cachedUnitPtr, lex_result, ast_result);
        } else {
            // since the unit is invalid, we should remove it from cache
            cache.cached_units.erase(found);
        }
    }

    // cached ast import unit
    auto cached_unit = std::make_shared<ASTImportUnit>(get_target_triple(), loc_man, typeBuilder);
    auto& comptime_scope = cached_unit->comptime_scope;

    // get the lex import unit
    auto lex_result = get_lexed(path);

    // get the ast unit
    auto ast_unit = get_ast(lex_result.get(), comptime_scope);

    // store cached ast import unit
    cache.cached_units.emplace(path, cached_unit);

    // return the complete unit ref
    return ASTImportUnitRef(false, path, mod, cached_unit, lex_result, ast_unit);

}

void build_diags_from_unit_ref(std::vector<std::vector<Diag>*>& diag_ptrs, ASTImportUnitRef& ref) {

    // parsing diagnostics for the last file (if parsing was done)
    diag_ptrs.emplace_back(&ref.ast_result->diags);

    // last file symbol res diagnostics
    if(!ref.unit->sym_res_diag.empty()) {
        diag_ptrs.emplace_back(&ref.unit->sym_res_diag);
    }

}

void WorkspaceManager::publish_diagnostics_for_sync(
    ASTImportUnitRef& ref,
    bool notify_async
) {
    std::vector<std::vector<Diag>*> diag_ptrs;
    build_diags_from_unit_ref(diag_ptrs, ref);
    notify_diagnostics(ref.path, diag_ptrs, true);
}

std::future<void>  WorkspaceManager::publish_diagnostics_for_async(ASTImportUnitRef& ref) {
    return std::async(std::launch::async, [this, ref]() mutable {
        std::vector<std::vector<Diag>*> diag_ptrs;
        build_diags_from_unit_ref(diag_ptrs, ref);
        notify_diagnostics_async(ref.path, diag_ptrs);
    });
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

void WorkspaceManager::publish_diagnostics(const std::string& path) {

    std::lock_guard<std::mutex> lock(publish_diagnostics_mutex);

    // Signal the current task to cancel if it's running
    if (publish_diagnostics_task.valid()) {
        publish_diagnostics_cancel_flag.store(true);
        publish_diagnostics_task.wait();  // Ensure the previous task has completed before launching a new one
    }

    // Reset the cancel flag for the new task
    publish_diagnostics_cancel_flag.store(false);

    // launch the task
    publish_diagnostics_task = std::async(std::launch::async, [path, this]() {
        // get the import unit ref
        auto import_unit = get_ast_import_unit(path, publish_diagnostics_cancel_flag);
        auto& unit = *import_unit.unit;

        // publish diagnostics for import unit (if not cached and job not cancelled)
        if(!publish_diagnostics_cancel_flag.load() && (!import_unit.is_cached || !unit.reported_diagnostics)) {
            publish_diagnostics_for_sync(import_unit, true);
            unit.reported_diagnostics = true;
        }

        // return the import unit
        return import_unit;
    });

}

std::vector<SemanticToken> WorkspaceManager::get_semantic_tokens(LexResult& file) {

#if defined PRINT_TOKENS && PRINT_TOKENS
    printTokens(file.unit.tokens);
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
    analyzer.analyze(file.tokens);
    return std::move(analyzer.tokens);

}