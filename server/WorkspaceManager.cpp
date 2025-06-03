// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 21/02/2024.
//

#include "WorkspaceManager.h"
#include "stream/SourceProvider.h"
#include <filesystem>
#include <sstream>
#include "server/analyzers/FoldingRangeAnalyzerApi.h"
#include "server/analyzers/CompletionItemAnalyzer.h"
#include "server/analyzers/GotoDefAnalyzer.h"
#include "server/analyzers/HoverAnalyzer.h"
#include "server/analyzers/DocumentSymbolsAnalyzer.h"
#include "server/analyzers/DocumentLinksAnalyzer.h"
#include "compiler/SelfInvocation.h"
#include "utils/PathUtils.h"
#include "stream/StringInputSource.h"
#include "compiler/lab/LabBuildCompiler.h"
#include "compiler/processor/ASTFileMetaData.h"
#include "compiler/lab/LabBuildContext.h"
#include "compiler/lab/LabBuildCompilerOptions.h"
#include "compiler/ASTProcessor.h"
#include "server/analyzers/InlayHintAnalyzerApi.h"
#include "server/analyzers/SignatureHelpAnalyzer.h"
#include <iostream>
#include <fstream>

#define DEBUG_REPLACE false

struct LSPLabImpl {

    LabBuildContext context;
    TCCState* state = nullptr;

    ~LSPLabImpl() {
        if(state) {
            tcc_delete(state);
        }
    }

};

WorkspaceManager::WorkspaceManager(
        std::string lsp_exe_path,
        lsp::MessageHandler& handler
) : lsp_exe_path(std::move(lsp_exe_path)), binder(compiler_exe_path()), handler(handler),
    global_allocator(10000), typeBuilder(global_allocator), pathHandler(compiler_exe_path()) {

}

LabModule* WorkspaceManager::get_mod_of(const chem::string_view& filePath) {
    auto found = filesIndex.find(filePath);
    return found != filesIndex.end() ? found->second : nullptr;
}

std::future<void> WorkspaceManager::trigger_sym_res(LabModule* module) {
    return std::async(std::launch::async, [this, module]() {

    });
}

std::string WorkspaceManager::get_target_triple() {
    // TODO we should get the current target triple from the compiler executable
    // then we store that target triple as the default, for example on windows
    // windows things will be shown
    return "LSP";
}

std::string WorkspaceManager::compiler_exe_path() {
#ifdef DEBUG
    std::string exe_name = "Compiler";
#else
    std::string exe_name = "chemical";
#endif
#if defined(_WIN32)
    return resolve_rel_parent_path_str(lsp_exe_path, exe_name + ".exe");
#else
    return resolve_rel_parent_path_str(lsp_exe_path, exe_name);
#endif
}

std::string WorkspaceManager::resources_path() {
    if(overridden_resources_path.empty()) {
        return resources_path_rel_to_exe(lsp_exe_path);
    } else {
        return overridden_resources_path;
    }
}

std::string WorkspaceManager::get_mod_file_path(){
    return resolve_sibling(project_path, "chemical.mod");
}

std::string WorkspaceManager::get_build_lab_path(){
    return resolve_sibling(project_path, "build.lab");
}

void WorkspaceManager::switch_main_job(LabJob* job) {
    main_job = job;
}

void WorkspaceManager::post_build_lab(LabBuildCompiler* compiler) {

    const auto& build = *lab;
    auto& context = build.context;
    if(!context.executables.empty()) {
        // using the first job as the main job
        switch_main_job(context.executables.front().get());
    }

    compiler->do_allocating((void*) &context, [](LabBuildCompiler* compiler, void* data) -> int {
        auto& context = *((LabBuildContext*) data);
        for(auto& job : context.executables) {
            if(job->type == LabJobType::CBI) {
                compiler->do_job(job.get());
            }
        }
        return 0;
    });

    // lets index all the files to modules they belong to
    for(auto& mod_ptr : modStorage.get_modules()) {
        const auto mod = mod_ptr.get();

        // determine each module files
        ASTProcessor::determine_module_files(pathHandler, loc_man, mod);

        // index each module files
        for(auto& file : mod->direct_files) {
            filesIndex.emplace(chem::string_view(file.abs_path), mod);
        }
    }

}

bool WorkspaceManager::compile_build_lab() {
    auto mod_file = get_mod_file_path();
    if(std::filesystem::exists(mod_file)) {

        auto build_dir = resolve_sibling(mod_file, "build");
        LabBuildCompilerOptions options(compiler_exe_path(), "ide", build_dir, is64Bit);
        LabBuildCompiler compiler(binder, &options);
        auto& storage = modStorage;
        const auto impl = new LSPLabImpl { LabBuildContext(compiler, pathHandler, storage, binder, mod_file), nullptr };
        ModuleDependencyRecord record("", chem::string(""), chem::string(""));
        auto module = compiler.built_mod_file(impl->context, mod_file);
        if(module == nullptr) {
            return false;
        }

        // let's create a single job that depends on this module
        chem::string abs_path(resolve_rel_child_path_str(
                build_dir,
#ifdef _WIN32
                "output.exe"
#else
                "output"
#endif
        ));
        auto job = new LabJob(LabJobType::Executable, module->name.copy(), std::move(abs_path), chem::string(build_dir));
        impl->context.executables.emplace_back(job);

        post_build_lab(&compiler);
        return true;

    }
    auto lab_path = get_build_lab_path();
    if(std::filesystem::exists(lab_path)) {

        auto build_dir = resolve_sibling(lab_path, "build");
        LabBuildCompilerOptions options(compiler_exe_path(), "ide", build_dir, is64Bit);
        LabBuildCompiler compiler(binder, &options);
        auto& storage = modStorage;
        const auto impl = new LSPLabImpl { LabBuildContext(compiler, pathHandler, storage, binder, lab_path), nullptr };
        ModuleDependencyRecord record("", chem::string(""), chem::string(""));
        const auto state = compiler.built_lab_file(impl->context, record, lab_path);

        // get the build method
        auto build = (void(*)(LabBuildContext*)) tcc_get_symbol(state, "build");
        if(!build) {
            // there's no build function in the build.lab
            return false;
        }

        // call the root build.lab build's function
        build(&impl->context);

        if(state) {

            // set the state as active
            impl->state = state;
            lab = impl;

            // post build lab
            post_build_lab(&compiler);

            return true;
        } else {
            delete impl;
            // TODO report failure to ide
            return false;
        }

    } else {
        return false;
    }
}

//void WorkspaceManager::initialize(const td_initialize::request &req) {
//    project_path = canonical_path(req.params.rootUri->GetAbsolutePath().path);
//    // doing asynchronous tasks during initialization
//    std::future<void> futureObj = std::async(std::launch::async, [this] {
//        // compile build.lab asynchronously
//        compile_build_lab();
//    });
//}

std::optional<std::string> WorkspaceManager::get_overridden_source(const std::string &path) {
    if (overriddenSources.contains(path)) {
        return overriddenSources[path];
    } else {
        return std::nullopt;
    }
}

std::vector<lsp::FoldingRange> WorkspaceManager::get_folding_range(const std::string_view& path) {
    const auto abs_path = canonical(path);
    auto unit = get_decl_ast(abs_path);
    return folding_analyze(loc_man, unit->unit.scope.body.nodes);
}
//
//td_completion::response WorkspaceManager::get_completion(
//        const lsDocumentUri& uri,
//        unsigned int line,
//        unsigned int character
//) {
//    auto can_path = canonical(uri.GetAbsolutePath().path);
//    auto unit = get_ast_import_unit(can_path, cancel_request);
//    CompletionItemAnalyzer analyzer(loc_man, { line, character });
//    td_completion::response rsp;
//    analyzer.analyze(unit);
//    rsp.result = std::move(analyzer.list);
//    return std::move(rsp);
//}
//
//td_links::response WorkspaceManager::get_links(const lsDocumentUri& uri) {
//    auto result = get_lexed(canonical(uri.GetAbsolutePath().path));
//    DocumentLinksAnalyzer analyzer;
//    td_links::response rsp;
//    rsp.result = analyzer.analyze(result.get(), compiler_exe_path(), lsp_exe_path);
//    return std::move(rsp);
//}
//
//td_inlayHint::response WorkspaceManager::get_hints(const lsDocumentUri& uri) {
//    const auto abs_path = canonical(uri.GetAbsolutePath().path);
//    auto result = get_ast_import_unit(abs_path, cancel_request);
//    td_inlayHint::response rsp;
//    rsp.result = inlay_hint_analyze(loc_man, result, compiler_exe_path(), lsp_exe_path);
//    return std::move(rsp);
//}
//
//td_signatureHelp::response WorkspaceManager::get_signature_help(const lsDocumentUri& uri, const lsPosition& position) {
//    const auto abs_path = canonical(uri.GetAbsolutePath().path);
//    auto result = get_ast_import_unit(abs_path, cancel_request);
//    SignatureHelpAnalyzer analyzer(loc_man, { .line = position.line, .character = position.character });
//    td_signatureHelp::response rsp;
//    analyzer.analyze(result);
//    rsp.result = std::move(analyzer.help);
//    return std::move(rsp);
//}
//
//td_definition::response WorkspaceManager::get_definition(const lsDocumentUri &uri, const lsPosition &position) {
//    auto unit = get_ast_import_unit(canonical(uri.GetAbsolutePath().path), cancel_request);
//    GotoDefAnalyzer analyzer(loc_man, {position.line, position.character});
//    td_definition::response rsp;
//    rsp.result.first.emplace();
//    auto analyzed = analyzer.analyze(unit.lex_result.get());
//    for (auto &loc: analyzed) {
//        rsp.result.first.value().push_back(lsLocation{
//                lsDocumentUri(AbsolutePath(loc.path)),
//                {
//                        {static_cast<int>(loc.range.start.line), static_cast<int>(loc.range.start.character)},
//                        {static_cast<int>(loc.range.end.line), static_cast<int>(loc.range.end.character)}
//                }
//        });
//    }
//    return rsp;
//}
//
std::vector<lsp::DocumentSymbol> WorkspaceManager::get_symbols(const std::string_view& path) {
    const auto abs_path = canonical(path);
    auto ast = get_decl_ast(abs_path);
    DocumentSymbolsAnalyzer analyzer(loc_man);
    analyzer.analyze(ast->unit.scope.body.nodes);
    return std::move(analyzer.symbols);
}
//
//td_hover::response WorkspaceManager::get_hover(const lsDocumentUri& uri, const lsPosition& position) {
//    auto unit = get_ast_import_unit(canonical(uri.GetAbsolutePath().path), cancel_request);
//    td_hover::response rsp;
//    HoverAnalyzer analyzer(loc_man, {position.line, position.character});
//    auto value = analyzer.markdown_hover(unit.lex_result.get());
//    if(!value.empty()) {
//        rsp.result.contents.second.emplace("markdown", std::move(value));
//    }
//    return rsp;
//}

void WorkspaceManager::onChangedContents(
        const std::string_view &abs_path,
        const std::vector<lsp::TextDocumentContentChangeEvent> &changes
) {

    // no changes return !
    if (changes.empty()) {
        std::cerr << "onChangedContents: zero changes present" << std::endl;
        return;
    }

    auto path = canonical(abs_path);

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
            // invalidate the cached file for this key
            cache.files_ast.erase(path);
            cache.cached_units.erase(path);
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

    // invalidate the cached file for this key
    cache.files_ast.erase(path);
    cache.cached_units.erase(path);

}

void WorkspaceManager::onClosedFile(const std::string &path) {
    overriddenSources.erase(path);
}

void WorkspaceManager::clearAllStoredContents() {
    overriddenSources.clear();
}

WorkspaceManager::~WorkspaceManager() {
    delete lab;
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