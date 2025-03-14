// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 10/03/2024.
//

#include "utils/FileUtils.h"
#include "utils/Utils.h"
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
    // preparing tokens for the last file
    SemanticTokens tokens;
    try {
        // .get causes no state exception sometimes
        auto unit = publish_diagnostics_task.get();
        auto& files = unit.lex_unit.files;
        auto last_file = files.empty() ? get_lexed(abs_path) : files[files.size() - 1];
        auto toks = get_semantic_tokens(*last_file);
        tokens.data = SemanticTokens::encodeTokens(toks);
    } catch(const std::exception& e) {
        std::cerr << "error when getting semantic tokens: " << e.what() << std::endl;
        auto last_file = get_lexed(abs_path);
        auto toks = get_semantic_tokens(*last_file);
        tokens.data = SemanticTokens::encodeTokens(toks);
    }
    // sending tokens
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
    char resolver_memory[resolver_mem_size];
    // all heap allocations will not be batched
    ASTAllocator resolver_allocator(resolver_memory, resolver_mem_size, 0);

    // let's do symbol resolution
    SymbolResolver resolver(
            comptime_scope,
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
        resolver.resolve_file(file->unit.scope, file->abs_path);
        resolver.diagnostics.clear();
        i++;
    }

    // check it hasn't been cancelled
    if(cancel_flag.load()) {
        return {};
    }

    // doing last file
    auto& last_file = ast_files[last];
    resolver.resolve_file(last_file->unit.scope, last_file->abs_path);

    return std::move(resolver.diagnostics);

}

ASTImportUnitRef WorkspaceManager::get_ast_import_unit(
    const std::string& path,
    std::atomic<bool>& cancel_flag
) {

    // lock for single invocation
    std::lock_guard lock(ast_import_unit_mutex);

    // check the cache
    auto found = cache.cached_units.find(path);
    if(found != cache.cached_units.end()) {
        auto cachedUnitPtr = found->second;
        auto& cachedUnit = *cachedUnitPtr;
        ASTImportUnitRef importUnit(true, path, cachedUnitPtr);
        // check that every file in cached import unit is valid
        bool is_unit_valid = true;
        for(auto& file : cachedUnit.lex_files) {
            auto ptr = file.lock();
            if(ptr) {
                importUnit.lex_unit.files.emplace_back(ptr);
            } else {
                is_unit_valid = false;
                break;
            }
        }
        if(is_unit_valid) {
            for (auto& file: cachedUnit.files) {
                auto ptr = file.lock();
                if (ptr) {
                    importUnit.files.emplace_back(ptr);
                } else {
                    is_unit_valid = false;
                    break;
                }
            }
        }
        if(is_unit_valid) {
            return importUnit;
        } else {
            // since the unit is invalid, we should remove it from cache
            cache.cached_units.erase(found);
        }
    }

    // cached ast import unit
    auto cached_unit = std::make_shared<ASTImportUnit>(get_target_triple(), loc_man);
    auto& comptime_scope = cached_unit->comptime_scope;

    // get the lex import unit
    auto import_unit = get_import_unit(path, cancel_flag);

    // put lex files in cached ast unit
    auto& unit_lex_files = cached_unit->lex_files;
    for(auto& file : import_unit.files) {
        unit_lex_files.emplace_back(file);
    }

    // check it hasn't been cancelled
    if(cancel_flag.load()) {
        return ASTImportUnitRef(false, path, cached_unit, std::move(import_unit));
    }

    // whether to cache the ast unit
    bool cache_it = true;

    // get the ast import unit
    std::vector<std::shared_ptr<ASTResult>> ast_files;
    get_ast_import_unit(ast_files, import_unit, comptime_scope, cancel_flag);
    // put ast files in cached ast unit
    auto& unit_ast_files = cached_unit->files;
    for(auto& file : ast_files) {
        if(file) {
            unit_ast_files.emplace_back(file);
        } else {
            cache_it = false;
        }
    }

    // check it hasn't been cancelled
    if(cancel_flag.load()) {
        return ASTImportUnitRef(false, path, cached_unit, std::move(import_unit), ast_files);
    }

    // symbol resolve the import unit, get the last file's diagnostics
    cached_unit->sym_res_diag = sym_res_import_unit(ast_files, comptime_scope, cancel_flag);

    if(cache_it) { // since imported files are missing, we must not cache the import unit
        // store cached ast import unit
        cache.cached_units.emplace(path, cached_unit);
    }

    // return the complete unit ref
    return ASTImportUnitRef(false, path, cached_unit, std::move(import_unit), ast_files);

}

void build_diags_from_unit_ref(std::vector<std::vector<Diag>*>& diag_ptrs, ASTImportUnitRef& ref) {

    // import graph diagnostics (report which files weren't found)
    auto& ig_root = ref.lex_unit.ig_root;
    if(!ig_root.errors.empty()) {
        diag_ptrs.emplace_back(&ig_root.errors);
    }
    auto& ig_files = ig_root.files;
    if(!ig_files.empty()) {
        auto& last_errors = ig_files[ig_files.size() - 1].errors;
        if(!last_errors.empty()) {
            diag_ptrs.emplace_back(&last_errors);
        }
    }

    // lex diagnostics for the last file
    auto& lex_files = ref.lex_unit.files;
    if(!lex_files.empty()) {
        auto& last_file_diags = lex_files[lex_files.size() - 1]->diags;
        diag_ptrs.emplace_back(&last_file_diags);
    }

    // parsing diagnostics for the last file (if parsing was done)
    auto& ast_files = ref.files;
    if(!ast_files.empty()) {
        auto& last_file_diags = ast_files[ast_files.size() - 1]->diags;
        diag_ptrs.emplace_back(&last_file_diags);
    }

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
    analyzer.analyze(file.unit.tokens);
    return std::move(analyzer.tokens);

}