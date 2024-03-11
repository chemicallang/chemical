// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/03/2024.
//

#include "LspSemanticTokens.h"
#include "SemanticLinker.h"
#include "utils/JsonUtils.h"
#include "utils/FileUtils.h"
#include "utils/Utils.h"

#define DEBUG false
#define PRINT_TOKENS false

std::vector<SemanticToken> to_semantic_tokens(FileTracker &tracker, const std::string &path) {

    auto overridden = tracker.get_overridden_source(path);

    auto lexed = tracker.getLexedFile(path);

    SemanticLinker linker(lexed);

    linker.analyze();

    if(PRINT_TOKENS) {
        printTokens(lexed, linker.resolved);
    }

    if(DEBUG) {
        if(overridden.has_value()) {
            // Writing the source code to a debug file
            writeToProjectFile("debug/source.txt", overridden.value());
            // Writing the source code as ascii to a debug file
            writeAsciiToProjectFile("debug/ascii.txt", overridden.value());
        }
    }

    if(DEBUG) {
        JsonUtils utils;
        utils.serialize("debug/tokens.json", lexed);
    }

    std::vector<SemanticToken> tokens;

    unsigned int prevTokenStart = 0;
    unsigned int prevTokenLineNumber = 0;
    unsigned int i = 0;
    while (i < lexed.size()) {

        auto token = lexed[i].get();
        auto found = linker.resolved.find(i);
        auto resolved = found != linker.resolved.end() ? linker.tokens[found->second].get() : token;

        tokens.push_back(SemanticToken{
                token->lineNumber() - prevTokenLineNumber, (
                        token->lineNumber() == prevTokenLineNumber ? (
                                // on the same line
                                token->lineCharNumber() - prevTokenStart
                        ) : (
                                // on a different line
                                token->lineCharNumber()
                        )
                ), token->length(), static_cast<unsigned int>(resolved->lspType()), resolved->lsp_modifiers()
        });
        prevTokenStart = token->lineCharNumber();
        prevTokenLineNumber = token->lineNumber();

        i++;
    }

    return tokens;

}