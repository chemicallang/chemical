// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 10/03/2024.
//

#include "utils/FileUtils.h"
#include "utils/PathUtils.h"
#include "lsp/types.h"
#include "lsp/messages.h"
#include "server/analyzers/SemanticTokensAnalyzer.h"
#include "WorkspaceManager.h"
#include "compiler/SymbolResolver.h"
#include <future>
#include <iostream>
#include <filesystem>
#include "lsp/messagehandler.h"
#include "lexer/Lexer.h"
#include "stream/InputSource.h"
#include "parser/Parser.h"
#include "compiler/ASTProcessor.h"
#include "compiler/symres/NodeSymbolDeclarer.h"
#include <functional>
#include <utility>

#define DEBUG_TOKENS false
#define PRINT_TOKENS false

void add_diagnostics(std::vector<lsp::Diagnostic> &diagnostics, std::vector<Diag>& diags) {
    for(auto& diag : diags) {
        diagnostics.emplace_back(lsp::Diagnostic(
                lsp::Range(
                        lsp::Position(diag.range.start.line, diag.range.start.character),
                        lsp::Position(diag.range.end.line, diag.range.end.character)
                ),
                diag.message,
                static_cast<lsp::DiagnosticSeverity>(static_cast<int>(diag.severity.value()))
        ));
    }
}

void build_diagnostics(std::vector<lsp::Diagnostic> &diagnostics, const std::vector<std::vector<Diag>*>& diags) {
    for(const auto diagsPtr : diags) {
        add_diagnostics(diagnostics, *diagsPtr);
    }
}

bool parse_file(
        WorkspaceManager& manager,
        ASTAllocator& allocator,
        LocationManager& loc_man,
        ASTUnit& unit,
        unsigned int fileId,
        const std::string_view& abs_path
) {

    auto& binder = manager.binder;
    auto& typeBuilder = manager.typeBuilder;
    const auto is64Bit = manager.is64Bit;

    auto abs_path_str = std::string(abs_path);

    LexResult lexResult;
    if(!manager.get_lexed(&lexResult, abs_path_str, false)) {
        return false;
    }

    auto& tokens = lexResult.tokens;

    // do not continue, if error occurs during lexing
    if(lexResult.has_errors) {
        return false;
    }

    // parse the file
    Parser parser(
            fileId,
            abs_path,
            tokens.data(),
            loc_man,
            allocator,
            allocator,
            typeBuilder,
            is64Bit,
            &binder
    );

    // setting file scope as parent of all nodes parsed
    parser.parent_node = &unit.scope;

    // actual parsing
    parser.parse(unit.scope.body.nodes);

    if(parser.has_errors) {
        return false;
    } else {
        return true;
    }

}

void parseModule(
        WorkspaceManager& manager,
        LocationManager& loc_man,
        LabModule* module,
        ModuleScope* modScope,
        std::vector<CachedASTUnitRef>& units
) {

    for(auto& file : module->direct_files) {

        auto file_path_view = chem::string_view(file.abs_path);

        auto found = manager.cachedUnits.find(file_path_view);
        if(found != manager.cachedUnits.end()) {
            units.emplace_back(CachedASTUnitRef {
                .unit = found->second.unit.get(),
                .is_symbol_resolved = found->second.is_symbol_resolved
            });
        } else {

            // every file costs us 10kb batched allocator
            // which means for module of 100 files, 1mb for each module
            // for 2000 modules, we will allocate 2gb (+100mb metadata) on average
            ASTAllocator allocator(10000);

            const auto unitPtr = new ASTUnit(file.file_id, file_path_view, modScope);

            parse_file(manager, allocator, loc_man, *unitPtr, file.file_id, file.abs_path);

            manager.cachedUnits.emplace(file_path_view, CachedASTUnit{
                    .allocator = std::move(allocator), // 10kb batched allocator
                    .unit = std::unique_ptr<ASTUnit>(unitPtr),
                    .is_symbol_resolved = false
            });

            units.emplace_back(CachedASTUnitRef {
                    .unit = unitPtr,
                    .is_symbol_resolved = false
            });

        }

    }

}

void parseModule(
        WorkspaceManager& manager,
        LocationManager& loc_man,
        LabModule* module,
        ModuleData* modData,
        std::vector<CachedASTUnitRef>& units
) {
    return parseModule(manager, loc_man, module, modData->modScope, units);
}

ModuleData* getModuleData(WorkspaceManager& manager, LabModule* module) {
    if(!module) return nullptr;
    auto found = manager.moduleData.find(module);
    if(found != manager.moduleData.end()) {
        return found->second.get();
    } else {
        const auto modDataPtr = new ModuleData(nullptr);
        const auto modScope = new (modDataPtr->allocator.allocate<ModuleScope>()) ModuleScope(module->scope_name.to_chem_view(), module->name.to_chem_view(), module);
        modDataPtr->modScope = modScope;
        manager.moduleData.emplace(module, modDataPtr);
        return modDataPtr;
    }
}

//std::pair<LabModule*, std::vector<ASTUnit>> parseModuleHelper2(
//        int id,
//        WorkspaceManager& manager,
//        ASTAllocator& allocator,
//        LocationManager& loc_man,
//        LabModule* module
//) {
//    return { module, parseModuleHelper(manager, allocator, loc_man, module) };
//}

//void parseModuleHelper3(
//        int id,
//        WorkspaceManager& manager,
//        ASTAllocator& allocator,
//        LocationManager& loc_man,
//        LabModule* module,
//        CachedModuleUnit& modUnit
//) {
//
//    return { module, parseModuleHelper(manager, allocator, loc_man, module, modUnit.fileUnits) };
//}

//inline void push(
//        std::vector<std::future<void>>& unitsFutures,
//        WorkspaceManager& manager,
//        ASTAllocator& allocator,
//        LocationManager& loc_man,
//        LabModule* module
//) {
//    unitsFutures.emplace_back(
//            manager.pool.push(parseModuleHelper3, std::ref(manager), std::ref(allocator), std::ref(loc_man), module)
//    );
//}

//inline std::future<std::pair<LabModule*, std::vector<ASTUnit>>> push(
//        std::vector<std::future<std::pair<LabModule*, std::vector<ASTUnit>>>>& unitsFutures,
//        WorkspaceManager& manager,
//        ASTAllocator& allocator,
//        LocationManager& loc_man,
//        LabModule* module
//) {
//    return manager.pool.push(parseModuleHelper2, std::ref(manager), std::ref(allocator), std::ref(loc_man), module);
//}

//void parseModuleParallel(
//        std::vector<std::future<std::pair<LabModule*, std::vector<ASTUnit>>>>& unitsFutures,
//        WorkspaceManager& manager,
//        ASTAllocator& allocator,
//        LocationManager& loc_man,
//        LabModule* module
//) {
//    for(const auto dep : module->dependencies) {
//        parseModuleParallel(unitsFutures, manager, allocator, loc_man, dep);
//    }
//    unitsFutures.emplace_back(push(unitsFutures, manager, allocator, loc_man, module));
//}

struct ModuleUnitWithDeps {

    CachedModuleUnit unit;

    std::vector<ModuleUnitWithDeps> dependencies;

    ModuleData* modData;

    /**
     * an empty module unit with deps
     */
    ModuleUnitWithDeps(LabModule* module, ModuleData* modData) : unit(module), modData(modData) {

    }

};

void parseModuleWithDeps(
        int id,
        WorkspaceManager& manager,
        LabModule* module,
        ModuleData* modData,
        ModuleUnitWithDeps& modWithDeps
) {

    std::vector<std::future<void>> unitsFutures;
    auto& depsUnits = modWithDeps.dependencies;

    for(const auto dep : module->dependencies) {

        const auto depModData = getModuleData(manager, module);
        depsUnits.emplace_back(module, depModData);
        auto& depUnit = depsUnits.back();

        auto future = manager.pool.push(parseModuleWithDeps, std::ref(manager), dep, depModData, std::ref(depUnit));
        unitsFutures.emplace_back(std::move(future));

    }

    // lets wait for all dependencies to finish
    for(auto& future : unitsFutures) {
        future.get();
    }

    parseModule(manager, manager.loc_man, module, modData, modWithDeps.unit.fileUnits);

}

void sym_res_mod_sig(SymbolResolver& resolver, ModuleUnitWithDeps& modWithDeps) {

    const auto mod_index = resolver.module_scope_start();

    // this is an important step, to switch the allocators
    const auto resolver_allocator = &modWithDeps.modData->allocator;
    resolver.ast_allocator = resolver_allocator;
    resolver.mod_allocator = resolver_allocator;

    // declaring symbols of direct dependencies
    SymbolResolverDeclarer declarer(resolver);
    for(auto& depUnit : modWithDeps.dependencies) {
        for(auto& cachedUnit : depUnit.unit.fileUnits) {
            for(const auto node : cachedUnit.unit->scope.body.nodes) {
                declare_node(declarer, node, AccessSpecifier::Public);
            }
        }
    }

    // symbol resolving all the files in the module
    auto& modUnit = modWithDeps.unit;

    // private symbol ranges are stored in this vector
    std::vector<SymbolRange> priv_sym_ranges;
    priv_sym_ranges.reserve(modUnit.fileUnits.size());

    // declaring symbols of all files
    for(auto& cachedUnit : modUnit.fileUnits) {

        const auto unit = cachedUnit.unit;
        auto path_str = unit->scope.file_path.str();

        auto priv_sym_range = resolver.tld_declare_file(unit->scope.body, path_str);
        priv_sym_ranges.emplace_back(priv_sym_range);

    }

    // linking signatures in all files
    unsigned i = 0;
    for(auto& cachedUnit : modUnit.fileUnits) {

        const auto unit = cachedUnit.unit;
        auto path_str = unit->scope.file_path.str();

        auto& priv_sym_range = priv_sym_ranges[i];

        resolver.link_signature_file(unit->scope.body, path_str, priv_sym_range);

        i++;
    }

    // ending the scope, drops all the symbols from these modules
    resolver.module_scope_end(mod_index);

}

void sym_res_mod_sig_recursive(
        int id,
        WorkspaceManager& manager,
        SymbolResolver& resolver,
        ModuleUnitWithDeps& modWithDeps,
        bool& symbol_resolve_flag
) {
    std::vector<std::future<void>> unitsFutures;

    for(auto& dep : modWithDeps.dependencies) {

        auto future = manager.pool.push(sym_res_mod_sig_recursive, std::ref(manager), std::ref(resolver), std::ref(dep), std::ref(symbol_resolve_flag));
        unitsFutures.emplace_back(std::move(future));

    }

    // lets wait for all dependencies to finish
    for(auto& future : unitsFutures) {
        future.get();
    }

    if(!symbol_resolve_flag && !modWithDeps.unit.all_files_symbol_resolved()) {
        // force symbol resolution, if one of the file is not symbol resolved
        symbol_resolve_flag = true;
    }

    if(!symbol_resolve_flag) return;

    // symbol resolve the signature of the module
    sym_res_mod_sig(resolver, modWithDeps);
}


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
std::vector<uint32_t> WorkspaceManager::get_semantic_tokens_full(const std::string_view& path) {

    auto str_path = std::string(path);
    auto abs_path = canonical(path);
    // tokens for the last file
    auto last_file = get_lexed(abs_path, true);
    if(!last_file) {
        return {};
    }

    auto& all_tokens = last_file->tokens;

    // copy the tokens
    std::vector<Token> copied_tokens;
    copied_tokens.reserve(all_tokens.size());

    // index for each token is stored (index in all_tokens)
    std::vector<size_t> originalIndexes;
    originalIndexes.reserve(all_tokens.size());

    // copy the tokens and record the original indexes
    size_t i = 0;
    for(auto& token : all_tokens) {
        if(token.type != TokenType::SingleLineComment && token.type != TokenType::MultiLineComment) {
            copied_tokens.emplace_back(token);
            originalIndexes.emplace_back(i);
        }
        i++;
    }

    // parse the copied tokens
    auto ast = get_ast_no_lock(copied_tokens.data(), str_path);

    // now restore the mapping of ast with tokens (using original indexes)
    i = 0;
    for(auto& token : copied_tokens) {
        if(token.linked != nullptr) {
            auto original_index = originalIndexes[i];
            auto& originalToken = all_tokens[original_index];
            originalToken.linked = token.linked;
        }
        i++;
    }

    // simply symbol resolve the ast
    const unsigned int resolver_mem_size = 10000; // pre allocated 10kb
    // all heap allocations will not be batched
    ASTAllocator resolver_allocator(resolver_mem_size);

    // a comptime scope is required for comptime things
    // TODO use a output mode according properly
    GlobalInterpretScope comptime_scope(OutputMode::Debug, "lsp", nullptr, nullptr, resolver_allocator, typeBuilder, loc_man);

    // let's do symbol resolution
    SymbolResolver resolver(
            comptime_scope,
            pathHandler,
            is64Bit,
            resolver_allocator,
            &resolver_allocator,
            &resolver_allocator
    );

    // prepare top level compiler functions
    if(global_container) {
        comptime_scope.rebind_container(resolver, global_container);
    } else {
        global_container = comptime_scope.create_container(resolver);
    }

    // got the module
    const auto mod = get_mod_of(chem::string_view(abs_path));

    // get the module data
    const auto modData = mod ? getModuleData(*this, mod) : nullptr;

    // this should be created here (so its destroyed after we've created the tokens)
    ModuleUnitWithDeps modWithDeps(mod, modData);

    if(mod) {

        // trigger parse of module with dependencies
        parseModuleWithDeps(0, *this, mod, modData, modWithDeps);

        // this flag determines whether we should symbol resolve the modules
        // if in case one of module hasn't been symbol resolved then it and all its dependencies are resolved
        // this flag means, ignore cache, symbol resolve forcefully, if true
        bool symbol_resolve_flag = false;

        // symbol resolve (declare + link signature) of dependencies (recursively) (NOT current module)
        for(auto& unit : modWithDeps.dependencies) {
            bool sym_res_dep_flag = true;
            sym_res_mod_sig_recursive(0, *this, resolver, unit, sym_res_dep_flag);
            // if any of the dependency is being symbol resolved (may have changed)
            // then we will symbol resolve the current module too
            if(sym_res_dep_flag) {
                symbol_resolve_flag = true;
            }
        }

        // force symbol resolve the module, if even one of the file is not symbol resolved
        if(!symbol_resolve_flag && !modWithDeps.unit.all_files_symbol_resolved()) {
            symbol_resolve_flag = true;
        }

        // only symbol resolve if required (one of deps / current module file changed)
        if(symbol_resolve_flag) {

            // declaring symbols of direct dependencies
            SymbolResolverDeclarer declarer(resolver);
            for (auto& depUnit: modWithDeps.dependencies) {
                for (auto& cachedUnit: depUnit.unit.fileUnits) {
                    const auto unit = cachedUnit.unit;
                    for (const auto node: unit->scope.body.nodes) {
                        declare_node(declarer, node, AccessSpecifier::Public);
                    }
                }
            }

            std::vector<SymbolRange> priv_sym_ranges(modWithDeps.unit.fileUnits.size());

            // declaring top level symbols of all files in module
            auto curr_file_path = std::filesystem::path(path);
            i = 0;
            for (auto& cachedUnit: modWithDeps.unit.fileUnits) {
                const auto unit = cachedUnit.unit;
                auto file_path_str = unit->scope.file_path.str();
                if (!std::filesystem::equivalent(curr_file_path, file_path_str)) {
                    priv_sym_ranges[i] = resolver.tld_declare_file(unit->scope.body, file_path_str);
                }
                i++;
            }

            // linking signatures of all files in current module
            i = 0;
            for (auto& cachedUnit: modWithDeps.unit.fileUnits) {
                const auto unit = cachedUnit.unit;
                auto file_path_str = unit->scope.file_path.str();
                if (!std::filesystem::equivalent(curr_file_path, file_path_str)) {
                    resolver.link_signature_file(unit->scope.body, file_path_str, priv_sym_ranges[i]);
                }
            }

        }

        // must reset the diagnostics, so that we don't report diagnostics for other files
        resolver.reset_diagnostics();

    }

    // declare and link file
    resolver.declare_and_link_file(ast->unit.scope.body, str_path);

    // building the diagnostics
    std::vector<lsp::Diagnostic> diagnostics;
    add_diagnostics(diagnostics, last_file->diags);
    add_diagnostics(diagnostics, ast->diags);
    add_diagnostics(diagnostics, resolver.diagnostics);

    // publish diagnostics will return ast import unit ref
    publish_diagnostics(str_path, std::move(diagnostics));

    // report the tokens
    auto toks = get_semantic_tokens(*last_file);
    return std::move(toks);
}

void WorkspaceManager::index_new_file(const std::string_view& path) {
    // TODO index the file, find the module ( don't know how ) and put it in the module
    // TODO also index with the module pointer
    std::cout << "[lsp] created file '" << path << "', TODO: index it" << std::endl;
}

void WorkspaceManager::de_index_deleted_file(const std::string_view& path_sv) {
    namespace fs = std::filesystem;
    fs::path deletedPath(path_sv);
    // Find owning module
    if (const auto mod = get_mod_of(chem::string_view(path_sv))) {
        auto& files = mod->direct_files;
        for (auto it = files.begin(); it != files.end(); ++it) {
            fs::path candidate(it->abs_path);
            // Compare actual filesystem locations (handles case, symlinks, etc.)
            std::error_code ec;
            if (fs::equivalent(candidate, deletedPath, ec) && !ec) {
                // Erase and return early
                files.erase(it);
                std::cout << "[lsp] de-indexed deleted file '" << path_sv << "' from module '" << mod->name << "'\n";
                return;
            }
        }
        std::cout << "[lsp] deleted file '" << path_sv << "' was not found in module '" << mod->name << "'\n";
    } else {
        std::cout << "[lsp] deleted file '" << path_sv << "' does not belong to any known module\n";
    }
}

void WorkspaceManager::register_watched_files_capability() {

    auto watcher = lsp::json::Object();
    watcher["globPattern"] = "**/*.{ch,mod}";
    watcher["kind"] = lsp::toJson(static_cast<unsigned int>(lsp::WatchKind::Create) | static_cast<unsigned int>(lsp::WatchKind::Delete));

    auto watchFileRegOpts = lsp::json::Object();
    watchFileRegOpts["watchers"] = lsp::json::Array({
        std::move(watcher)
    });

    auto params2 = lsp::requests::Client_RegisterCapability::Params();
    params2.registrations.emplace_back(lsp::Registration(
            "workspace/didChangeWatchedFiles",
            "workspace/didChangeWatchedFiles",
            std::move(watchFileRegOpts)
    ));

    auto msgId = handler.sendRequest<lsp::requests::Client_RegisterCapability>(std::move(params2));

}

void WorkspaceManager::notify_diagnostics(
        const std::string& path,
        std::vector<lsp::Diagnostic> diagnostics
) {
    auto params = lsp::notifications::TextDocument_PublishDiagnostics::Params{
            lsp::FileUri::fromPath(path), std::move(diagnostics), std::nullopt
    };
    handler.sendNotification<lsp::notifications::TextDocument_PublishDiagnostics>(std::move(params));
}

void WorkspaceManager::notify_diagnostics_async(
    const std::string& path,
    const std::vector<std::vector<Diag>*>& diags
) {
    std::vector<lsp::Diagnostic> diagnostics_list;
    build_diagnostics(diagnostics_list, diags);
    std::async(std::launch::async, [this, path, diagnostics = std::move(diagnostics_list)] () mutable {
        notify_diagnostics(path, std::move(diagnostics));
    });
}

void WorkspaceManager::notify_diagnostics_sync(
    const std::string& path,
    const std::vector<std::vector<Diag>*>& diags
) {
    std::vector<lsp::Diagnostic> diagnostics;
    build_diagnostics(diagnostics, diags);
    notify_diagnostics(path, std::move(diagnostics));
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

std::shared_ptr<ASTResult> WorkspaceManager::get_decl_ast(const std::string& path) {

    // get the module for the given file
    // TODO: use the module
    const auto mod = get_mod_of(chem::string_view(path));

    // get the lex import unit
    auto lex_result = get_lexed(path);

    // get the ast unit
    auto ast_unit = get_ast_no_lock(lex_result->tokens.data(), path);

    return ast_unit;

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

    // lex diagnostics
    diag_ptrs.emplace_back(&ref.lex_result->diags);

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

void WorkspaceManager::publish_diagnostics(const std::string& path, std::vector<lsp::Diagnostic> diagnostics) {

    std::lock_guard<std::mutex> lock(publish_diagnostics_mutex);

    // Signal the current task to cancel if it's running
    if (publish_diagnostics_task.valid()) {
        publish_diagnostics_cancel_flag.store(true);
        publish_diagnostics_task.wait();  // Ensure the previous task has completed before launching a new one
    }

    // Reset the cancel flag for the new task
    publish_diagnostics_cancel_flag.store(false);

    // launch the task
    publish_diagnostics_task = std::async(std::launch::async, [this, path, diagnostics = std::move(diagnostics)]() mutable -> void {
        if(!publish_diagnostics_cancel_flag.load()) {
            notify_diagnostics(path, std::move(diagnostics));
        }
    });

}

void WorkspaceManager::publish_diagnostics(std::shared_ptr<LexResult> file) {

    std::lock_guard<std::mutex> lock(publish_diagnostics_mutex);

    // Signal the current task to cancel if it's running
    if (publish_diagnostics_task.valid()) {
        publish_diagnostics_cancel_flag.store(true);
        publish_diagnostics_task.wait();  // Ensure the previous task has completed before launching a new one
    }

    // Reset the cancel flag for the new task
    publish_diagnostics_cancel_flag.store(false);

    // launch the task
    publish_diagnostics_task = std::async(std::launch::async, [file, this]() -> void {
        auto ast = get_ast_no_lock(file->tokens.data(), file->abs_path);
        // publish diagnostics for import unit (if not cached and job not cancelled)
        if(!publish_diagnostics_cancel_flag.load()) {
            std::vector<std::vector<Diag>*> diag_ptrs{ &file->diags, &ast->diags };
            notify_diagnostics_sync(file->abs_path, diag_ptrs);
        }
    });

}

std::vector<uint32_t> WorkspaceManager::get_semantic_tokens(LexResult& file) {

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