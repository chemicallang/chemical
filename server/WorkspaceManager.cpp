// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 21/02/2024.
//

#include "WorkspaceManager.h"
#include "stream/SourceProvider.h"
#include <sstream>
#include "LibLsp/lsp/textDocument/foldingRange.h"
#include "server/analyzers/FoldingRangeAnalyzer.h"
#include "LibLsp/lsp/textDocument/completion.h"
#include "LibLsp/lsp/textDocument/document_link.h"
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

#define DEBUG_REPLACE false

std::optional<std::string> WorkspaceManager::get_overridden_source(const std::string &path) {
    if (overriddenSources.contains(path)) {
        return overriddenSources[path];
    } else {
        return std::nullopt;
    }
}

td_foldingRange::response WorkspaceManager::get_folding_range(const std::string &abs_path) {
    td_foldingRange::response rsp;
    auto &tokens = get_lexed_tokens(abs_path);
    FoldingRangeAnalyzer analyzer;
    analyzer.analyze(tokens);
    rsp.result = std::move(analyzer.ranges);
    return rsp;
}

td_completion::response WorkspaceManager::get_completion(
        const std::string &abs_path,
        unsigned int line,
        unsigned int character
) {
    auto unit = get_import_unit(abs_path);
    CompletionItemAnalyzer analyzer({ line, character });
    td_completion::response rsp;
    rsp.result = analyzer.analyze(&unit);
    return std::move(rsp);
}

td_links::response WorkspaceManager::get_links(const lsDocumentUri& uri) {
    auto result = get_lexed(uri.GetAbsolutePath().path);
    DocumentLinksAnalyzer analyzer;
    td_links::response rsp;
    rsp.result = analyzer.analyze(result.get());
    return std::move(rsp);
}

td_definition::response WorkspaceManager::get_definition(const lsDocumentUri &uri, const lsPosition &position) {
    auto unit = get_import_unit(uri.GetAbsolutePath().path);
    GotoDefAnalyzer analyzer({position.line, position.character});
    td_definition::response rsp;
    rsp.result.first.emplace();
    auto analyzed = analyzer.analyze(&unit);
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
    auto& tokens = get_lexed_tokens(uri.GetAbsolutePath().path);
    DocumentSymbolsAnalyzer analyzer;
    td_symbol::response rsp;
    analyzer.analyze(tokens);
    rsp.result = std::move(analyzer.symbols);
    return rsp;
}

td_hover::response WorkspaceManager::get_hover(const lsDocumentUri& uri, const lsPosition& position) {
    auto unit = get_import_unit(uri.GetAbsolutePath().path);
    td_hover::response rsp;
    HoverAnalyzer analyzer({position.line, position.character});
    auto value = analyzer.markdown_hover(&unit);
    if(!value.empty()) {
        rsp.result.contents.second.emplace("markdown", std::move(value));
    }
    return rsp;
}

void
WorkspaceManager::onChangedContents(const std::string &path,
                                    const std::vector<lsTextDocumentContentChangeEvent> &changes) {

    // no changes return !
    if (changes.empty()) {
//        std::cout << "no changes in source code";
        return;
    }

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

}

void WorkspaceManager::onClosedFile(const std::string &path) {
    overriddenSources.erase(path);
}

void WorkspaceManager::clearAllStoredContents() {
    overriddenSources.clear();
}

void replace(
        std::string &source,
        unsigned int lineStart,
        unsigned int charStart,
        unsigned int lineEnd,
        unsigned int charEnd,
        const std::string &replacement
) {

    std::istringstream stream(source);

    auto provider = SourceProvider(stream);

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