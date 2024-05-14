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
#include "server/analyzers/CompletionItemAnalyzer.h"
#include "LibLsp/lsp/textDocument/SemanticTokens.h"
#include "server/LspSemanticTokens.h"

#define DEBUG false

Lexer WorkspaceManager::getLexedFile(const std::string &path) {
    if (overriddenSources.contains(path)) {
//        if(DEBUG) std::cout << "Retrieved Overridden Source:" << overriddenSources[path] << '\n';
        std::istringstream iss(overriddenSources[path]);
        return lexFile(iss, path);
    } else {
        if(DEBUG) std::cout << "No Overridden Source:" << path << '\n';
        return lexFile(path);
    }
}

std::optional<std::string> WorkspaceManager::get_overridden_source(const std::string& path) {
    if (overriddenSources.contains(path)) {
        return overriddenSources[path];
    } else {
        return std::nullopt;
    }
}

void WorkspaceManager::onChangedContents(const std::string &path, const std::string &contents) {
    overriddenSources[path] = contents;
}

td_foldingRange::response WorkspaceManager::get_folding_range(const std::string& abs_path) {
    td_foldingRange::response rsp;
    auto lexed = getLexedFile(abs_path);
    FoldingRangeAnalyzer analyzer;
    analyzer.analyze(lexed.tokens);
    rsp.result = std::move(analyzer.ranges);
    return rsp;
}

td_completion::response WorkspaceManager::get_completion(const std::string &abs_path, unsigned int line, unsigned int character) {
    auto lexed = getLexedFile(abs_path);
    CompletionItemAnalyzer analyzer(std::pair(line, character));
    td_completion::response rsp;
    rsp.result = analyzer.analyze(lexed.tokens);
    return std::move(rsp);
}

void
WorkspaceManager::onChangedContents(const std::string &path, const std::vector<lsTextDocumentContentChangeEvent> &changes) {

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

    if(DEBUG) std::cout << "loaded the source : " << source << std::endl;
    if(DEBUG) std::cout << "total changes :" << changes.size() << std::endl;

    if (DEBUG && changes.size() == 1) {
        auto change = changes[0];
        auto start = change.range.value().start;
        auto end = change.range.value().end;
        std::cout << " change : start : " << start.line << '-' << start.character << " end : " << end.line << '-'
                  << end.character << ";" << std::endl;
    }

    // make changes to the source code
    for (const auto &change: changes) {
        if (change.range.has_value()) {
            auto start = change.range.value().start;
            auto end = change.range.value().end;
            replaceSafe(source, start.line, start.character, end.line, end.character, change.text);
        }
    }


    if(DEBUG) std::cout << "replaced : " << source << std::endl;

    // store the overridden sources
    overriddenSources[path] = std::move(source);

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

    if(DEBUG) std::cout << "reading:";

    auto not_replaced = true;

    while (!provider.eof()) {
        if (not_replaced && (provider.getLineNumber() == lineStart && provider.getLineCharNumber() == charStart)) {

            // forwarding to the end without adding character
            if(DEBUG) std::cout << "[fwd]:[";
            while (!provider.eof() && !(provider.getLineNumber() == lineEnd && provider.getLineCharNumber() == charEnd)) {
                if(DEBUG) {
                    std::cout << provider.readCharacter();
                } else {
                    provider.readCharacter();
                }
            }
            if(DEBUG) std::cout << ']';

            // adding replacement
            nextSource += replacement;
            if(DEBUG) std::cout << "[rep]:[" << replacement << ']';

            // replaced
            not_replaced = false;

        } else {
            auto c = provider.readCharacter();
            nextSource += c;
            if(DEBUG) std::cout << c;
        }
    }

    if(DEBUG) std::cout << '\n';

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