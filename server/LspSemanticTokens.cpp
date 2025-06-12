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
        ASTUnit& unit,
        Token* start_token,
        std::vector<Diag>* outDiags
) {

    auto& binder = manager.binder;
    auto& typeBuilder = manager.typeBuilder;
    auto& loc_man = manager.loc_man;
    const auto is64Bit = manager.is64Bit;

    // parse the file
    Parser parser(
            unit.scope.file_id,
            unit.scope.file_path.view(),
            start_token,
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

    // if given we send back the diagnostics
    if(outDiags) {
        *outDiags = std::move(parser.diagnostics);
    }

    if(parser.has_errors) {
        return false;
    } else {
        return true;
    }

}

bool parse_file(
        WorkspaceManager& manager,
        ASTAllocator& allocator,
        ASTUnit& unit
) {

    auto abs_path_str = std::string(unit.scope.file_path.view());

    LexResult lexResult;
    if(!manager.get_lexed(&lexResult, abs_path_str, false)) {
        return false;
    }

    auto& tokens = lexResult.tokens;

    // do not continue, if error occurs during lexing
    if(lexResult.has_errors) {
        return false;
    }

    return parse_file(
        manager, allocator, unit, tokens.data(), nullptr
    );

}

void parseModule(
        WorkspaceManager& manager,
        LabModule* module,
        ModuleData* modData
) {

    // the units in the module (prepared once across multiple threads)
    auto& units = modData->fileUnits;

    // lets prepare the file units
    for(auto& file : module->direct_files) {

        auto file_path_view = chem::string_view(file.abs_path);

        const auto cachedUnit = new CachedASTUnit {
            .allocator = ASTAllocator(10000),
            .unit = ASTUnit(file.file_id, file_path_view, modData->modScope),
            .is_symbol_resolved = false
        };

        // every file costs us 10kb batched allocator
        // which means for module of 100 files, 1mb for each module
        // for 2000 modules, we will allocate 2gb (+100mb metadata) on average
        auto& allocator = cachedUnit->allocator;

        // parsing the file
        parse_file(manager, allocator, cachedUnit->unit);

        // putting the unit in module data
        modData->cachedUnits.emplace(file_path_view, std::unique_ptr<CachedASTUnit>(cachedUnit));
        units.emplace_back(cachedUnit);

    }

    // set it to true, important step for caching to work
    modData->prepared_file_units = true;

}

ModuleData* WorkspaceManager::getModuleData(LabModule* module) {
    std::lock_guard guard_creation(module_data_mutex);
    auto found = moduleData.find(module);
    if(found != moduleData.end()) {
        return found->second.get();
    } else {
        const auto modDataPtr = new ModuleData(nullptr);
        const auto modScope = new (modDataPtr->allocator.allocate<ModuleScope>()) ModuleScope(module->scope_name.to_chem_view(), module->name.to_chem_view(), module);
        modDataPtr->modScope = modScope;
        moduleData.emplace(module, modDataPtr);
        return modDataPtr;
    }
}

void parseModuleWithDeps(
        int id,
        WorkspaceManager& manager,
        LabModule* module,
        ModuleData* modData,
        std::vector<std::future<void>>& futures
) {

    // fast path, if already prepared file units, then we do not need to do it
    if(modData->prepared_file_units) {
        return;
    }

    // we lock the mutex for a single parse of this module
    std::lock_guard guard_parse(modData->module_mutex);

    // maybe some other thread prepared the file units (while we were trying to acquire lock)
    if(modData->prepared_file_units) {
        return;
    }

    // launch all modules concurrently and recursively
    // because in parsing, no module is dependent on another module
    for(const auto dep : module->dependencies) {

        // getting the module data for dependency module
        const auto depModData = manager.getModuleData(dep);

        // we are preparing dependencies of this module as well (once)
        modData->dependencies.emplace_back(depModData);

        // launching dependencies recursively with the same function
        futures.emplace_back(
                manager.pool.push(parseModuleWithDeps, std::ref(manager), dep, depModData, std::ref(futures))
        );

    }

    // actually parsing the module
    parseModule(manager, module, modData);

    // set it to true, important step for caching to work
    // doing it while the lock is still in place
    modData->prepared_file_units = true;

}

void parseModuleWithDeps(
        int id,
        WorkspaceManager& manager,
        LabModule* module,
        ModuleData* modData
) {
    std::vector<std::future<void>> futures;
    // parse
    parseModuleWithDeps(
        id,  manager, module, modData, futures
    );
    // wait
    for(auto& future : futures) {
        future.get();
    }
}

void sym_res_mod_sig(SymbolResolver& resolver, ModuleData* modData) {

    const auto mod_index = resolver.module_scope_start();

    // this is an important step, to switch the allocators
    const auto resolver_allocator = &modData->allocator;
    resolver.ast_allocator = resolver_allocator;
    resolver.mod_allocator = resolver_allocator;

    // get the file units
    auto& fileUnits = modData->fileUnits;

    // declaring symbols of direct dependencies
    SymbolResolverDeclarer declarer(resolver);
    for(const auto depUnit : modData->dependencies) {
        for(const auto cachedUnit : depUnit->fileUnits) {
            for(const auto node : cachedUnit->unit.scope.body.nodes) {
                declare_node(declarer, node, AccessSpecifier::Public);
            }
        }
    }

    // private symbol ranges are stored in this vector
    std::vector<SymbolRange> priv_sym_ranges;
    priv_sym_ranges.reserve(fileUnits.size());

    // declaring symbols of all files
    for(const auto cachedUnit : fileUnits) {

        auto& unit = cachedUnit->unit;
        auto path_str = unit.scope.file_path.str();

        auto priv_sym_range = resolver.tld_declare_file(unit.scope.body, path_str);
        priv_sym_ranges.emplace_back(priv_sym_range);

    }

    // linking signatures in all files
    unsigned i = 0;
    for(const auto cachedUnit : fileUnits) {

        auto& unit = cachedUnit->unit;
        auto path_str = unit.scope.file_path.str();

        auto& priv_sym_range = priv_sym_ranges[i];

        resolver.link_signature_file(unit.scope.body, path_str, priv_sym_range);

        i++;
    }

    // ending the scope, drops all the symbols from these modules
    resolver.module_scope_end(mod_index);

    // set that all files inside this module has symbol resolved
    for(const auto fileUnit : modData->fileUnits) {
        fileUnit->is_symbol_resolved = true;
    }

}

void sym_res_mod_sig_recursive(
        int id,
        WorkspaceManager& manager,
        SymbolResolver& resolver,
        ModuleData* modData,
        bool& symbol_resolve_flag
) {
    std::vector<std::future<void>> unitsFutures;

    for(const auto dep : modData->dependencies) {

        auto future = manager.pool.push(sym_res_mod_sig_recursive, std::ref(manager), std::ref(resolver), dep, std::ref(symbol_resolve_flag));
        unitsFutures.emplace_back(std::move(future));

    }

    // lets wait for all dependencies to finish
    for(auto& future : unitsFutures) {
        future.get();
    }

    if(!symbol_resolve_flag && !modData->all_files_symbol_resolved()) {
        // force symbol resolution, if one of the file is not symbol resolved
        symbol_resolve_flag = true;
    }

    if(!symbol_resolve_flag) return;

    // symbol resolve the signature of the module
    sym_res_mod_sig(resolver, modData);

}

void WorkspaceManager::bind_or_create_container(GlobalInterpretScope& comptime_scope, SymbolResolver& resolver) {

    // fast path, if container exists, rebind and return as fast as possible
    if(global_container) {
        comptime_scope.rebind_container(resolver, global_container);
        return;
    }

    // lock the creation process
    std::lock_guard lock_creation(global_container_mutex);

    // maybe someone created the container, while we were waiting to acquire the lock
    if(global_container) {
        comptime_scope.rebind_container(resolver, global_container);
        return;
    }

    // create the container
    global_container = comptime_scope.create_container(resolver);

}

void WorkspaceManager::process_file(const std::string_view& path, bool current_file_changed) {

    auto str_path = std::string(path);
    auto abs_path = canonical(path);

    // tokens for the last file
    auto last_file = get_lexed(abs_path, true);
    if(!last_file) {
        // couldn't get tokens for the file
        // maybe a read error or something similar
        return;
    }

    auto abs_path_view = chem::string_view(abs_path);
    auto& all_tokens = last_file->tokens;

    // store the tokens in token cache
    tokenCache.put(abs_path, last_file);

    // if there was an error during lexing, or if it ended unexpectedly
    // we do not proceed with parsing and report lexing diagnostics
    if(last_file->has_errors) {

        // publish the diagnostics up until now
        std::vector<lsp::Diagnostic> diagnostics;
        add_diagnostics(diagnostics, last_file->diags);
        publish_diagnostics(str_path, std::move(diagnostics));

        // return early, as we don't want to proceed with parsing
        return;

    }

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

    // batch heap allocations, pre allocated 10kb
    const unsigned int resolver_mem_size = 10000;
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
            // these allocators are switched before symbol resolution for each module
            &resolver_allocator,
            &resolver_allocator
    );

    // prepare top level compiler functions (std, compiler namespace)
    bind_or_create_container(comptime_scope, resolver);

    // got the module
    const auto mod = get_mod_of(abs_path_view);

    // get the module data
    const auto modData = mod ? getModuleData(mod) : nullptr;

    // must hold parse diagnostics from different sources
    std::vector<Diag> parse_diagnostics;

    if(mod) {

        // trigger parse of module with dependencies
        parseModuleWithDeps(0, *this, mod, modData);

        // this flag determines whether we should symbol resolve the modules
        // if in case one of module hasn't been symbol resolved then it and all its dependencies are resolved
        // this flag means, ignore cache, symbol resolve forcefully, if true
        bool symbol_resolve_flag = false;

        // symbol resolve (declare + link signature) of dependencies (recursively) (NOT current module)
        for(const auto depData : modData->dependencies) {
            bool sym_res_dep_flag = false;
            sym_res_mod_sig_recursive(0, *this, resolver, depData, sym_res_dep_flag);
            // if any of the dependency is being symbol resolved (may have changed)
            // then we will symbol resolve the current module too
            if(sym_res_dep_flag) {
                symbol_resolve_flag = true;
            }
        }

        // just a better name for symbol resolve flag
        // this means should symbol resolve current module (except current file) ?
        auto& sym_res_curr_mod = symbol_resolve_flag;

        // since direct dependencies say they have been symbol resolved
        // therefore not to symbol resolve the current modules
        // force symbol resolve the module, if even one of the file is not symbol resolved
        // (except current file, because we always symbol resolve that)
        if(!sym_res_curr_mod) {

            // then we must check if any of its own file changed for the final verdict
            auto curr_file_path = std::filesystem::path(path);
            for(const auto fileUnit : modData->fileUnits) {
                if (!fileUnit->is_symbol_resolved && !std::filesystem::equivalent(curr_file_path, fileUnit->unit.scope.file_path.str())) {
                    // this is not current file
                    // must symbol resolve, changed file is not current file
                    sym_res_curr_mod = true;
                    break;
                }
            }

            // since we did not symbol resolve direct dependencies
            // the problem is that they may contain modifications to std, compiler namespace
            // however when rebinding container, we removed these modifications
            // to enable nodes user added to std, compiler namespace, we must go over
            // nodes of direct dependencies and add them manually
            for(const auto depData : modData->dependencies) {
                for(const auto cachedUnit : depData->fileUnits) {
                    auto& unit = cachedUnit->unit;
                    for(const auto node : unit.scope.body.nodes) {
                        if(node->kind() == ASTNodeKind::NamespaceDecl) {
                            node->as_namespace_unsafe()->extend_root_namespace();
                        }
                    }
                }
            }

        }

        // must switch the allocators before performing symbol resolution for this module
        // so any created types during linking remain on its allocator
        resolver.mod_allocator = &modData->allocator;
        resolver.ast_allocator = &modData->allocator;

        // declaring symbols of direct dependencies
        SymbolResolverDeclarer declarer(resolver);
        for (const auto depData: modData->dependencies) {
            for (const auto cachedUnit: depData->fileUnits) {
                auto& unit = cachedUnit->unit;
                for (const auto node: unit.scope.body.nodes) {
                    declare_node(declarer, node, AccessSpecifier::Public);
                }
            }
        }

        // only symbol resolve if required (one of deps / current module file changed)
        if(sym_res_curr_mod) {

            // a container for private symbol ranges (of files)
            std::vector<SymbolRange> priv_sym_ranges(modData->fileUnits.size());

            // declaring top level symbols of all files in module
            auto curr_file_path = std::filesystem::path(path);
            i = 0;
            for (const auto cachedUnit: modData->fileUnits) {
                auto& unit = cachedUnit->unit;
                auto file_path_str = unit.scope.file_path.str();
                if (!std::filesystem::equivalent(curr_file_path, file_path_str)) {
                    priv_sym_ranges[i] = resolver.tld_declare_file(unit.scope.body, file_path_str);
                }
                i++;
            }

            // linking signatures of all files in current module
            i = 0;
            for (const auto cachedUnit: modData->fileUnits) {
                auto& unit = cachedUnit->unit;
                auto file_path_str = unit.scope.file_path.str();
                if (!std::filesystem::equivalent(curr_file_path, file_path_str)) {
                    resolver.link_signature_file(unit.scope.body, file_path_str, priv_sym_ranges[i]);
                }
            }

        }

        // must reset the diagnostics, so that we don't report diagnostics for other files
        resolver.reset_diagnostics();

        auto found = modData->cachedUnits.find(abs_path_view);
        if(found != modData->cachedUnits.end()) {

            auto& cachedUnit = *found->second;
            auto& allocator = cachedUnit.allocator;
            auto& astUnit = cachedUnit.unit;

            // if the current file didn't change
            // we don't need to even reparse it
            // we can just resolve symbols inside again (to provide)
            // diagnostics
            if(current_file_changed) {

                // clear the previous unit
                allocator.clear();
                astUnit.scope.body.nodes.clear();

                // parse the new tokens into the ast unit
                // we need to parse, because the parseModule above won't parse any file
                // since module has already (probably) prepared the file units (done once)
                parse_file(*this, allocator, astUnit, copied_tokens.data(), &parse_diagnostics);

                // we will set this file symbol resolved = false
                // so next time a file is opened that depends on the module that contains this file
                // it resolves dependencies, before resolving the file
                cachedUnit.is_symbol_resolved = false;

            }

            // declare and link file
            resolver.declare_and_link_file(astUnit.scope.body, str_path);

        } else if(modData->prepared_file_units) {
            // we have prepared the file units, however the file that belongs to this module
            // is not present inside the cachedUnits
#ifdef DEBUG
            throw std::runtime_error("prepared file units, however current file not present");
#endif
        }

    } else {

        // creating a 10kb allocator for parsing this file
        ASTAllocator allocator(10000);

        // this file doesn't belong to any module (or we don't know about it)
        const auto anonymous_scope = new (allocator.allocate<ModuleScope>()) ModuleScope(chem::string_view(""), chem::string_view("anon"), nullptr);

        // encode the file path to get file id
        const auto fileId = loc_man.encodeFile(abs_path);

        // the temporary ast unit
        ASTUnit astUnit(fileId, abs_path_view, anonymous_scope);

        // parsing the file to unit
        parse_file(*this, allocator, astUnit, copied_tokens.data(), &parse_diagnostics);

        // declare and link file
        resolver.declare_and_link_file(astUnit.scope.body, str_path);

    }

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

    // building the diagnostics
    std::vector<lsp::Diagnostic> diagnostics;
    add_diagnostics(diagnostics, last_file->diags);
    add_diagnostics(diagnostics, parse_diagnostics);
    add_diagnostics(diagnostics, resolver.diagnostics);

    // publish diagnostics will return ast import unit ref
    publish_diagnostics(str_path, std::move(diagnostics));

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

    // check if tokens exist in cache (parsed after changed contents request of file)
    auto cachedTokens = tokenCache.get(abs_path);
    if(cachedTokens != nullptr) {
        return get_semantic_tokens(**cachedTokens);
    }

    // tokens for the last file
    auto last_file = get_lexed(abs_path, true);
    if(!last_file) {
        return {};
    }

    // report the tokens
    return get_semantic_tokens(*last_file);

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

ASTUnit* WorkspaceManager::get_stored_unit(const std::string_view& path) {

    auto path_view = chem::string_view(path);
    const auto mod = get_mod_of(path_view);
    if(mod) {
        const auto modData = getModuleData(mod);
        if(modData) {
            auto found = modData->cachedUnits.find(path_view);
            if(found != modData->cachedUnits.end()) {
                return &found->second->unit;
            }
        }
    }

    return nullptr;

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