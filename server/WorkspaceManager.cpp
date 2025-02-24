// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 21/02/2024.
//

#include "WorkspaceManager.h"
#include "stream/SourceProvider.h"
#include <filesystem>
#include <sstream>
#include "LibLsp/lsp/textDocument/foldingRange.h"
#include "server/analyzers/FoldingRangeAnalyzer.h"
#include "LibLsp/lsp/textDocument/completion.h"
#include "LibLsp/lsp/textDocument/document_link.h"
#include "LibLsp/lsp/textDocument/inlayHint.h"
#include "LibLsp/lsp/textDocument/signature_help.h"
#include "server/analyzers/CompletionItemAnalyzer.h"
#include "LibLsp/lsp/textDocument/SemanticTokens.h"
#include "LibLsp/lsp/textDocument/did_change.h"
#include "LibLsp/lsp/textDocument/declaration_definition.h"
#include "LibLsp/lsp/AbsolutePath.h"
#include "server/analyzers/GotoDefAnalyzer.h"
#include "LibLsp/lsp/textDocument/hover.h"
#include "server/analyzers/HoverAnalyzer.h"
#include "server/analyzers/DocumentSymbolsAnalyzer.h"
#include "server/analyzers/DocumentLinksAnalyzer.h"
#include "compiler/SelfInvocation.h"
#include "utils/PathUtils.h"
#include "stream/StringInputSource.h"
#include "LibLsp/lsp/general/initialize.h"
#include "compiler/lab/LabBuildCompiler.h"
#include "compiler/lab/LabBuildContext.h"
#include "compiler/lab/LabBuildCompilerOptions.h"
#include "server/analyzers/InlayHintAnalyzer.h"
#include "server/analyzers/SignatureHelpAnalyzer.h"

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

WorkspaceManager::WorkspaceManager(std::string lsp_exe_path) : lsp_exe_path(std::move(lsp_exe_path)), binder(compiler_exe_path()) {

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

}

bool WorkspaceManager::compile_build_lab() {
    auto lab_path = get_build_lab_path();
    if(std::filesystem::exists(lab_path)) {

        LabBuildCompilerOptions options(compiler_exe_path(), "ide", is64Bit);
        LabBuildCompiler compiler(binder, &options);
        const auto impl = new LSPLabImpl { LabBuildContext(&options, lab_path, resolve_sibling(lab_path, "build")), nullptr };
        const auto state = compiler.built_lab_file(impl->context, lab_path);

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

void WorkspaceManager::initialize(const td_initialize::request &req) {
    project_path = canonical_path(req.params.rootUri->GetAbsolutePath().path);
    // doing asynchronous tasks during initialization
    std::future<void> futureObj = std::async(std::launch::async, [this] {
        // compile build.lab asynchronously
        compile_build_lab();
    });
}

std::pair<std::string, int> WorkspaceManager::get_c_translated(const std::string& header_abs_path, const std::string& output_path) {
    std::string output;
    std::vector<std::string> command = {compiler_exe_path(), '"' + header_abs_path + '"', "-o", '"' + output_path + '"', "-res", '"' + resources_path() + '"'};
    std::cout << "[LSP] Command : ";
    for(auto& cmd : command) {
        std::cout << cmd << ' ';
    }
    std::cout << std::endl;
    auto res = invoke_capturing_out(command, output);
    return {output, res};
}

std::optional<std::string> WorkspaceManager::get_overridden_source(const std::string &path) {
    if (overriddenSources.contains(path)) {
        return overriddenSources[path];
    } else {
        return std::nullopt;
    }
}

td_foldingRange::response WorkspaceManager::get_folding_range(const lsDocumentUri& uri) {
    td_foldingRange::response rsp;
    const auto abs_path = canonical(uri.GetAbsolutePath().path);
    auto unit = get_ast_import_unit(abs_path, cancel_request);
    FoldingRangeAnalyzer analyzer(loc_man);
    analyzer.analyze(unit.files.back()->unit.scope.nodes);
    rsp.result = std::move(analyzer.ranges);
    return rsp;
}

td_completion::response WorkspaceManager::get_completion(
        const lsDocumentUri& uri,
        unsigned int line,
        unsigned int character
) {
    auto can_path = canonical(uri.GetAbsolutePath().path);
    auto unit = get_ast_import_unit(can_path, cancel_request);
    CompletionItemAnalyzer analyzer(loc_man, { line, character });
    td_completion::response rsp;
    analyzer.analyze(unit);
    rsp.result = std::move(analyzer.list);
    return std::move(rsp);
}

td_links::response WorkspaceManager::get_links(const lsDocumentUri& uri) {
    auto result = get_lexed(canonical(uri.GetAbsolutePath().path));
    DocumentLinksAnalyzer analyzer;
    td_links::response rsp;
    rsp.result = analyzer.analyze(result.get(), compiler_exe_path(), lsp_exe_path);
    return std::move(rsp);
}

td_inlayHint::response WorkspaceManager::get_hints(const lsDocumentUri& uri) {
    const auto abs_path = canonical(uri.GetAbsolutePath().path);
    auto result = get_ast_import_unit(abs_path, cancel_request);
    InlayHintAnalyzer analyzer(loc_man);
    td_inlayHint::response rsp;
    rsp.result = analyzer.analyze(result, compiler_exe_path(), lsp_exe_path);
    return std::move(rsp);
}

td_signatureHelp::response WorkspaceManager::get_signature_help(const lsDocumentUri& uri, const lsPosition& position) {
    const auto abs_path = canonical(uri.GetAbsolutePath().path);
    auto result = get_ast_import_unit(abs_path, cancel_request);
    SignatureHelpAnalyzer analyzer(loc_man, { .line = position.line, .character = position.character });
    td_signatureHelp::response rsp;
    analyzer.analyze(result);
    rsp.result = std::move(analyzer.help);
    return std::move(rsp);
}

td_definition::response WorkspaceManager::get_definition(const lsDocumentUri &uri, const lsPosition &position) {
    auto unit = get_ast_import_unit(canonical(uri.GetAbsolutePath().path), cancel_request);
    GotoDefAnalyzer analyzer(loc_man, {position.line, position.character});
    td_definition::response rsp;
    rsp.result.first.emplace();
    auto analyzed = analyzer.analyze(&unit.lex_unit);
    for (auto &loc: analyzed) {
        rsp.result.first.value().push_back(lsLocation{
                lsDocumentUri(AbsolutePath(loc.path)),
                {
                        {static_cast<int>(loc.range.start.line), static_cast<int>(loc.range.start.character)},
                        {static_cast<int>(loc.range.end.line), static_cast<int>(loc.range.end.character)}
                }
        });
    }
    return rsp;
}

td_symbol::response WorkspaceManager::get_symbols(const lsDocumentUri& uri) {
    const auto abs_path = canonical(uri.GetAbsolutePath().path);
    auto unit = get_ast_import_unit(abs_path, cancel_request);
    DocumentSymbolsAnalyzer analyzer(loc_man);
    td_symbol::response rsp;
    analyzer.analyze(unit.files.back()->unit.scope.nodes);
    rsp.result = std::move(analyzer.symbols);
    return rsp;
}

td_hover::response WorkspaceManager::get_hover(const lsDocumentUri& uri, const lsPosition& position) {
    auto unit = get_ast_import_unit(canonical(uri.GetAbsolutePath().path), cancel_request);
    td_hover::response rsp;
    HoverAnalyzer analyzer(loc_man, {position.line, position.character});
    auto value = analyzer.markdown_hover(&unit.lex_unit);
    if(!value.empty()) {
        rsp.result.contents.second.emplace("markdown", std::move(value));
    }
    return rsp;
}

std::string WorkspaceManager::canonical(const std::string& path) {
    try {
        return std::filesystem::canonical(((std::filesystem::path) path)).string();
    } catch (std::filesystem::filesystem_error& e) {
        std::cerr << "[LSP_ERROR] onChangedContents : couldn't determine canonical path for " << path << std::endl;
        return "";
    }
}

void WorkspaceManager::onChangedContents(
        const lsDocumentUri &uri,
        const std::vector<lsTextDocumentContentChangeEvent> &changes
) {

    // no changes return !
    if (changes.empty()) {
//        std::cout << "no changes in source code";
        return;
    }

    auto path = canonical(uri.GetAbsolutePath().path);

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

#if defined DEBUG_REPLACE && DEBUG_REPLACE
    std::cout << "loaded the source : " << source << std::endl;
    std::cout << "total changes :" << changes.size() << std::endl;
    if(changes.size() == 1) {
        auto change = changes[0];
        auto start = change.range.value().start;
        auto end = change.range.value().end;
        std::cout << " change : start : " << start.line << '-' << start.character << " end : " << end.line << '-'
                  << end.character << ";" << std::endl;
    }
#endif

    // make changes to the source code
    for (const auto &change: changes) {
        if (change.range.has_value()) {
            auto start = change.range.value().start;
            auto end = change.range.value().end;
            replaceSafe(source, start.line, start.character, end.line, end.character, change.text);
        }
    }

#if defined DEBUG_REPLACE && DEBUG_REPLACE
    std::cout << "replaced : " << source << std::endl;
#endif

    // store the overridden sources
    overriddenSources[path] = std::move(source);

    // invalidate the cached file for this key
    cache.files.erase(path);
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