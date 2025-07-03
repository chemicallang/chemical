// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 21/02/2024.
//

#include "WorkspaceManager.h"
#include "stream/SourceProvider.h"
#include <filesystem>
#include <sstream>
#include "server/analyzers/FoldingRangeAnalyzer.h"
#include "server/analyzers/CompletionItemAnalyzer.h"
#include "server/analyzers/GotoDefAnalyzer.h"
#include "server/analyzers/HoverAnalyzer.h"
#include "server/analyzers/DocumentSymbolsAnalyzer.h"
#include "server/analyzers/DocumentLinksAnalyzer.h"
#include "compiler/SelfInvocation.h"
#include "utils/PathUtils.h"
#include "stream/StringInputSource.h"
#include "compiler/lab/LabBuildCompiler.h"
#include "ast/base/GlobalInterpretScope.h"
#include "compiler/processor/ASTFileMetaData.h"
#include "compiler/lab/LabBuildContext.h"
#include "compiler/lab/LabBuildCompilerOptions.h"
#include "integration/libtcc/LibTccInteg.h"
#include "server/build/ChildProcessBuild.h"
#include "compiler/ASTProcessor.h"
#include "server/analyzers/InlayHintAnalyzerApi.h"
#include "server/analyzers/SignatureHelpAnalyzer.h"
#include <iostream>
#include <fstream>

#define DEBUG_REPLACE false

WorkspaceManager::WorkspaceManager(
        std::string lsp_exe_path,
        lsp::MessageHandler& handler
) : lsp_exe_path(std::move(lsp_exe_path)), binder(compiler_exe_path()), handler(handler),
    global_allocator(10000), typeBuilder(global_allocator), pathHandler(compiler_exe_path()),
    context(modStorage, binder), pool((int) std::thread::hardware_concurrency()), tokenCache(10),
    modFileData(10), anonFilesData(10)
{

}

std::string WorkspaceManager::get_target_triple() {
    // TODO we should get the current target triple from the compiler executable
    // then we store that target triple as the default, for example on windows
    // windows things will be shown
    return "LSP";
}

std::string WorkspaceManager::compiler_exe_path() {
    return lsp_exe_path + " cc";
}

std::string WorkspaceManager::resources_path() {
    if(overridden_resources_path.empty()) {
        return resources_path_rel_to_exe(lsp_exe_path);
    } else {
        return overridden_resources_path;
    }
}

std::string WorkspaceManager::get_mod_file_path(){
    return resolve_rel_child_path_str(project_path, "chemical.mod");
}

std::string WorkspaceManager::get_build_lab_path(){
    return resolve_rel_child_path_str(project_path, "build.lab");
}

void WorkspaceManager::switch_main_job(LabJob* job) {
    main_job = job;
}

void WorkspaceManager::index_module_files() {
    // lets index all the files to modules they belong to
    for(auto& mod_ptr : modStorage.get_modules()) {
        const auto mod = mod_ptr.get();

        // determine each module files
        ASTProcessor::determine_module_files(pathHandler, loc_man, mod);

        // get a module data from the module pointer
        const auto modData = getModuleData(mod);

        // index each module files
        for(auto& file : mod->direct_files) {
            filesIndex.emplace(chem::string_view(file.abs_path), modData);
        }
    }
}

void WorkspaceManager::post_build_lab() {

    if(!context.executables.empty()) {
        // using the first job as the main job
        switch_main_job(context.executables.front().get());
    }

    index_module_files();

    // before we get compiling, the modules

    // compile cbi jobs and have their tcc state ready for invocation
    // the tcc state will end up inside the compiler binder
    for(auto& job : context.executables) {
        if(job->type == LabJobType::CBI) {
            compile_cbi((LabJobCBI*) job.get());
        }
    }

}

inline void create_dir(const std::string& build_dir) {
    // create the build directory for this executable
    if (!std::filesystem::exists(build_dir)) {
        std::filesystem::create_directory(build_dir);
    }
}


int WorkspaceManager::compile_cbi(LabJobCBI* job) {

    // using is64Bit as true
    auto exe_path = compiler_exe_path();
    auto& compiler_exe_path = exe_path;
    auto root_build_dir = resolve_rel_child_path_str(project_path, "build");
    auto ide_build_dir = resolve_rel_child_path_str(root_build_dir, "ide");
    auto build_dir = resolve_rel_child_path_str(ide_build_dir, "cbi");

    // set the job's build directory
    job->build_dir.clear();
    job->build_dir.append(build_dir);
    job->build_dir.append('/');
    job->build_dir.append(job->name.to_chem_view());

    // create build directory before proceeding (if it doesn't exist)
    create_dir(root_build_dir);
    create_dir(ide_build_dir);
    create_dir(build_dir);

    // setup stuff
    LabBuildCompilerOptions options(compiler_exe_path, "ide", build_dir, is64Bit);
    LabBuildCompiler compiler(loc_man, binder, &options);
    compiler.container = global_container;

    // allocating ast allocators
    const auto job_mem_size = 100000; // 100 kb
    const auto mod_mem_size = 100000; // 100 kb
    const auto file_mem_size = 100000; // 100 kb
    ASTAllocator _job_allocator(job_mem_size);
    ASTAllocator _mod_allocator(mod_mem_size);
    ASTAllocator _file_allocator(file_mem_size);

    // the allocators that will be used for all jobs
    compiler.set_allocators(&_job_allocator, &_mod_allocator, &_file_allocator);

    // do the job, the tcc state will automatically end up in compiler binder
    return compiler.do_job(job);

}

LabBuildContext* WorkspaceManager::compile_lab(const std::string& exe_path, const std::string& lab_path, ModuleStorage& modStorage) {

    // using is64Bit as true
    auto is64Bit = true;
    auto compiler_exe_path = exe_path + " cc";
    auto is_mod_source = lab_path.ends_with(".mod");
    auto compiler_build_dir = resolve_sibling(lab_path, "build");
    auto build_dir = resolve_rel_child_path_str(compiler_build_dir, "ide");
    // create build directory before proceeding (if it doesn't exist)
    create_dir(compiler_build_dir);
    create_dir(build_dir);

    LabBuildCompilerOptions options(compiler_exe_path, "ide", build_dir, is64Bit);
    CompilerBinder binder(compiler_exe_path);
    LocationManager loc_man;
    LabBuildCompiler compiler(loc_man, binder, &options);
    auto& storage = modStorage;
    ImportPathHandler pathHandler(compiler_exe_path);
    auto context_ptr = new LabBuildContext(compiler, pathHandler, storage, binder, lab_path);
    std::unique_ptr<LabBuildContext> context(context_ptr);

    // allocating ast allocators
    const auto job_mem_size = 100000; // 100 kb
    const auto mod_mem_size = 100000; // 100 kb
    const auto file_mem_size = 100000; // 100 kb
    ASTAllocator _job_allocator(job_mem_size);
    ASTAllocator _mod_allocator(mod_mem_size);
    ASTAllocator _file_allocator(file_mem_size);

    // the allocators that will be used for all jobs
    compiler.set_allocators(&_job_allocator, &_mod_allocator, &_file_allocator);

    // build the lab file to a tcc state
    const auto state = compiler.built_lab_file(*context, lab_path, is_mod_source);

    // auto delte the tcc state
    TCCDeletor auto_delete(state);

    // get the build method
    auto build = (void(*)(LabBuildContext*)) tcc_get_symbol(state, "chemical_lab_build");
    if(!build) {
        std::cerr << "[lsp] there's no build function in the file" << std::endl;
        return nullptr;
    }

    // clear the module storage
    // these modules were created to facilitate the build.lab generation
    // if not cleared, these modules will interfere with modules created for executable
    context->storage.clear();

    // call the root build.lab build's function
    build(context.get());

    // returning the context
    if(state) {
        return context.release();
    } else {
        return nullptr;
    }

}

int WorkspaceManager::build_context_from_build_lab() {
    // doing asynchronous tasks during initialization
    std::future<void> futureObj = std::async(std::launch::async, [this] {
        auto mod_file = get_mod_file_path();
        if(std::filesystem::exists(mod_file)) {
            std::cout << "[lsp] found mod file at '" << mod_file << "', triggering build" << std::endl;
            auto result = launch_child_build(context, lsp_exe_path, mod_file);
            if(result == 0) {
                post_build_lab();
            } else {
                std::cerr << "[lsp] failed build '" << mod_file << "'" << std::endl;
            }
        }
        auto lab_path = get_build_lab_path();
        if(std::filesystem::exists(lab_path)) {
            std::cout << "[lsp] found lab file at '" << lab_path << "', triggering build" << std::endl;
            auto result = launch_child_build(context, lsp_exe_path, lab_path);
            if(result == 0) {
                post_build_lab();
            } else {
                std::cerr << "[lsp] failed build '" << lab_path << "'" << std::endl;
            }
        }
    });
    return 0;
}

void WorkspaceManager::initialize(const lsp::InitializeParams &params) {
    if(!params.rootUri.isNull()) {
        project_path = canonical_path(params.rootUri->path());
        // compile build.lab asynchronously
        build_context_from_build_lab();
    } else if(params.rootPath.has_value() && !params.rootPath->isNull())  {
        project_path = canonical_path(params.rootPath->value());
        // compile build.lab asynchronously
        build_context_from_build_lab();
    } else {
        // couldn't get project path, user must have opened a file
    }
    std::async(std::launch::async, [this] {
        register_watched_files_capability();
    });
}

std::optional<std::string> WorkspaceManager::get_overridden_source(const std::string &path) {
    if (overriddenSources.contains(path)) {
        return overriddenSources[path];
    } else {
        return std::nullopt;
    }
}

// analyzers that expect symbol resolved ast units, should call this function to get the ast
// if this doesn't provide then it means before request a symbol resolved ast wasn't cached
// if that's the case, analyzers should not run
ASTUnit* get_cached_unit(WorkspaceManager& manager, ModuleData* modData, const std::string& path) {
    if(modData) {
        auto found = modData->cachedUnits.find(chem::string_view(path));
        if(found != modData->cachedUnits.end()) {
            return &found->second->unit;
        }
    } else {
        // it maybe an anonymous file
        auto found = manager.anonFilesData.get(path);
        if(found != nullptr) {
            return &found->get()->unit;
        }
    }
    return nullptr;
}

std::vector<lsp::FoldingRange> WorkspaceManager::get_folding_range(const std::string_view& path) {
    auto abs_path = canonical(path);
    auto abs_path_view = chem::string_view(abs_path);
    const auto modData = getModuleData(abs_path_view);
    const auto mod = modData ? modData->getModule() : nullptr;
    process_file_on_request(abs_path, modData);
    const auto unit = get_cached_unit(*this, modData, abs_path);
    LexResult* lexResult = nullptr;
    // check if tokens exist in cache (parsed after changed contents request of file)
    auto cachedTokens = tokenCache.get(abs_path);
    if(cachedTokens != nullptr) {
        lexResult = cachedTokens->get();
    }
    // TODO anonymous files tokens should also be stored for this operation
    if(lexResult) {
        FoldingRangeAnalyzer analyzer;
        analyzer.analyze(lexResult->tokens);
        return std::move(analyzer.ranges);
    } else {
        return {};
    }
}

lsp::CompletionList WorkspaceManager::get_completion(const std::string_view& path, const Position& position) {
    auto abs_path = canonical(path);
    auto abs_path_view = chem::string_view(abs_path);
    const auto modData = getModuleData(abs_path_view);
    const auto mod = modData ? modData->getModule() : nullptr;
    process_file_on_request(abs_path, modData);
    const auto unit = get_cached_unit(*this, modData, abs_path);
    LexResult* lexResult = nullptr;
    // check if tokens exist in cache (parsed after changed contents request of file)
    auto cachedTokens = tokenCache.get(abs_path);
    if(cachedTokens != nullptr) {
        lexResult = cachedTokens->get();
    }
    CompletionItemAnalyzer analyzer(loc_man, position);
    if(unit) {
        analyzer.analyze(mod, modData, lexResult, unit);
    }
    return std::move(analyzer.list);
}

//td_links::response WorkspaceManager::get_links(const lsDocumentUri& uri) {
//    auto result = get_lexed(canonical(uri.GetAbsolutePath().path));
//    DocumentLinksAnalyzer analyzer;
//    td_links::response rsp;
//    rsp.result = analyzer.analyze(result.get(), compiler_exe_path(), lsp_exe_path);
//    return std::move(rsp);
//}

std::vector<lsp::InlayHint> WorkspaceManager::get_hints(const std::string_view& path, const Range& range) {
    const auto abs_path = canonical(path);
    auto abs_path_view = chem::string_view(abs_path);
    const auto modData = getModuleData(abs_path_view);
    process_file_on_request(abs_path, modData);
    const auto unit = get_cached_unit(*this, modData, abs_path);
    if(unit) {
        return inlay_hint_analyze(loc_man, unit->scope.body.nodes, range);
    } else {
        // since we couldn't find the unit, we will not provide inlay hints
        // it would be best not to provide these
        return {};
    }
}

lsp::SignatureHelp WorkspaceManager::get_signature_help(const std::string_view& path, const Position& position) {
    const auto abs_path = canonical(path);
    auto abs_path_view = chem::string_view(abs_path);
    const auto modData = getModuleData(abs_path_view);
    const auto mod = modData ? modData->getModule() : nullptr;
    process_file_on_request(abs_path, modData);
    const auto unit = get_cached_unit(*this, modData, abs_path);
    LexResult* lexResult = nullptr;
    // check if tokens exist in cache (parsed after changed contents request of file)
    auto cachedTokens = tokenCache.get(abs_path);
    if(cachedTokens != nullptr) {
        lexResult = cachedTokens->get();
    }
    SignatureHelpAnalyzer analyzer(loc_man, position);
    if(unit) {
        analyzer.analyze(mod, modData, lexResult, unit);
    }
    return std::move(analyzer.help);
}

std::vector<lsp::DefinitionLink> WorkspaceManager::get_definition(const std::string_view& path, const Position &position) {
    const auto abs_path = canonical(path);
    process_file_on_request(abs_path);
    // check if tokens exist in cache (parsed after changed contents request of file)
    auto cachedTokens = tokenCache.get(abs_path);
    if(cachedTokens != nullptr) {
        GotoDefAnalyzer analyzer(loc_man, {position.line, position.character});
        return analyzer.analyze(cachedTokens->get()->tokens);
    }
    return {};
}

std::vector<lsp::DocumentSymbol> WorkspaceManager::get_symbols(const std::string_view& path) {
    const auto abs_path = canonical(path);
    auto abs_path_view = chem::string_view(abs_path);
    const auto modData = getModuleData(abs_path_view);
    const auto mod = modData ? modData->getModule() : nullptr;
    process_file_on_request(abs_path, modData);
    const auto unit = get_cached_unit(*this, modData, abs_path);
    DocumentSymbolsAnalyzer analyzer(loc_man);
    if(unit) {
        analyzer.analyze(unit->scope.body.nodes);
    }
    return std::move(analyzer.symbols);
}

std::string WorkspaceManager::get_hover(const std::string_view& path, const Position& position) {
    const auto abs_path = canonical(path);
    process_file_on_request(abs_path);
    // check if tokens exist in cache (parsed after changed contents request of file)
    auto cachedTokens = tokenCache.get(abs_path);
    if(cachedTokens != nullptr) {
        HoverAnalyzer analyzer(loc_man, {position.line, position.character});
        return analyzer.markdown_hover(abs_path, cachedTokens->get()->tokens);
    }
    return "";
}

void WorkspaceManager::OnOpenedFile(const std::string_view& filePath) {
    auto abs_path = canonical_path(filePath);
    process_any_file_on_open(abs_path);
}

void WorkspaceManager::onChangedContents(
        const std::string_view &non_canon_path,
        const std::vector<lsp::TextDocumentContentChangeEvent> &changes
) {

    // no changes return !
    if (changes.empty()) {
        std::cerr << "onChangedContents: zero changes present" << std::endl;
        return;
    }

    auto path = canonical(non_canon_path);

    // locking the incremental change mutex, when the object is destroyed, lock is released
    // causing requests to this method be processed sequentially
    std::lock_guard<std::mutex> lock(incremental_change_mutex);

    std::string source;

    // load the file if it doesn't exist
    if (!overriddenSources.contains(path)) {
        std::ifstream file;
        file.open(path);
        if (!file.is_open()) {
            std::cerr << "Unknown error opening the file" << '\n';
            return;
        }
        source = "";
        while (!file.eof()) {
            source += file.get();
        }
        file.close();
    } else {
        source = overriddenSources[path];
    }

    if(changes.size() == 1) {
        auto changePtr = get_if<lsp::TextDocumentContentChangeEvent_Text>(&changes[0]);
        if(changePtr) {
            auto& change = *changePtr;
            overriddenSources[path] = change.text;
            // reprocess the file (re-parse and symbol resolve, reporting diagnostics)
            process_any_file(path, true, false);
            return;
        }
    }

#if defined DEBUG_REPLACE && DEBUG_REPLACE
    std::cout << "loaded the source : " << source << std::endl;
    std::cout << "total changes :" << changes.size() << std::endl;
    if(changes.size() == 1) {
        auto change = changes[0];
        auto changePtr = get_if<lsp::TextDocumentContentChangeEvent_Range_Text>(&change);
        if(changePtr) {
            auto start = changePtr->range.start;
            auto end = changePtr->range.end;
            std::cout << " change : start : " << start.line << '-' << start.character << " end : " << end.line << '-' << end.character << ";" << std::endl;
        }
    }
#endif

    // make changes to the source code
    for (auto &changeVar: changes) {
        auto changePtr = get_if<lsp::TextDocumentContentChangeEvent_Range_Text>(&changeVar);
        if(changePtr) {
            auto& change = *changePtr;
            auto& start = change.range.start;
            auto& end = change.range.end;
            replaceSafe(source, start.line, start.character, end.line, end.character, change.text);
        } else {
#ifdef DEBUG
            throw std::runtime_error("unhandled whole document change");
#endif
        }
    }

#if defined DEBUG_REPLACE && DEBUG_REPLACE
    std::cout << "replaced : " << source << std::endl;
#endif

    // store the overridden sources
    overriddenSources[path] = std::move(source);

    // reprocess the file (re-parse and symbol resolve, reporting diagnostics)
    process_any_file(path, true, false);

}

void WorkspaceManager::onSave(const std::string_view& uri) {
    if(project_path.empty()) return;
    try {
        if (uri.ends_with("chemical.mod") || uri.ends_with(".lab")) {
            // lets try to clear everything we have on modules
            modStorage.clear();
            moduleData.clear();
            tokenCache.clear();
            context.executables.clear();
            filesIndex.clear();

            // this will try to rebuild context from chemical.mod/build.lab file present in project_dir
            build_context_from_build_lab();
        }
    } catch(std::filesystem::filesystem_error& e) {
#ifdef DEBUG
        std::cerr << "couldn't resolve canonical path for the file '" << uri << "'" << std::endl;
#endif
    }
}

void WorkspaceManager::onClosedFile(const std::string &path) {
    overriddenSources.erase(path);
}

void WorkspaceManager::clearAllStoredContents() {
    overriddenSources.clear();
}

WorkspaceManager::~WorkspaceManager() {
    GlobalInterpretScope::dispose_container(global_container);
}

void replace(
        std::string &source,
        unsigned int lineStart,
        unsigned int charStart,
        unsigned int lineEnd,
        unsigned int charEnd,
        const std::string &replacement
) {

    StringInputSource input_source(source);
    auto provider = SourceProvider(&input_source);

    std::string nextSource;

    if (DEBUG_REPLACE) std::cout << "reading:";

    auto not_replaced = true;

    while (!provider.eof()) {
        if (not_replaced && (provider.getLineNumber() == lineStart && provider.getLineCharNumber() == charStart)) {

            // forwarding to the end without adding character
            if (DEBUG_REPLACE) std::cout << "[fwd]:[";
            while (!provider.eof() &&
                   !(provider.getLineNumber() == lineEnd && provider.getLineCharNumber() == charEnd)) {
                if (DEBUG_REPLACE) {
                    std::cout << provider.readCharacter();
                } else {
                    provider.readCharacter();
                }
            }
            if (DEBUG_REPLACE) std::cout << ']';

            // adding replacement
            nextSource += replacement;
            if (DEBUG_REPLACE) std::cout << "[rep]:[" << replacement << ']';

            // replaced
            not_replaced = false;

        } else {
            auto c = provider.readCharacter();
            nextSource += c;
            if (DEBUG_REPLACE) std::cout << c;
        }
    }

    if (DEBUG_REPLACE) std::cout << '\n';

    source = nextSource;

}

void replaceSafe(std::string &source, unsigned int lineStart, unsigned int charStart, unsigned int lineEnd,
                 unsigned int charEnd, const std::string &replacement) {

    if (lineStart == lineEnd) {
        if (charStart == charEnd) {
            // range is closed, do nothing
        } else if (charStart > charEnd) {
            // if start is larger than end, call replace accurately (swapping start with end)
            replace(source, lineEnd, charEnd, lineStart, charStart, replacement);
            return;
        }
    } else if (lineStart > lineEnd) {
        // if start is larger than end, call replace accurately (swapping start with end)
        replace(source, lineEnd, charEnd, lineStart, charStart, replacement);
        return;
    }

    replace(source, lineStart, charStart, lineEnd, charEnd, replacement);

}