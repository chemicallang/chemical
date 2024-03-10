// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/03/2024.
//

#include "LspSemanticTokens.h"
#include "SemanticLexer.h"
#include "utils/JsonUtils.h"
#include "utils/FileUtils.h"
#include "utils/Utils.h"

#define DEBUG false
#define PRINT_TOKENS false

std::vector<SemanticToken> to_semantic_tokens(FileTracker &tracker, const std::string &path) {

    auto overridden = tracker.get_overridden_source(path);

    std::unique_ptr<SemanticLexer> lexer;

    if (overridden.has_value()) {
        std::istringstream stream(overridden.value());
        StreamSourceProvider provider(stream);
        lexer = std::make_unique<SemanticLexer>(provider, path);
        lexer->lex();
    } else {
        std::ifstream file;
        file.open(path);
        if (!file.is_open()) {
            std::cerr << "Unknown error opening the file" << '\n';
        }
        StreamSourceProvider provider(file);
        lexer = std::make_unique<SemanticLexer>(provider, path);
        // lex all the tokens
        lexer->lex();
        file.close();
    }

    if(PRINT_TOKENS) {
        printTokens(lexer->tokens);
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
        utils.serialize("debug/tokens.json", lexer->tokens);
    }

    std::vector<SemanticToken> tokens;

    unsigned int prevTokenStart = 0;
    unsigned int prevTokenLineNumber = 0;
    unsigned int i = 0;
    while (i < lexer->tokens.size()) {

        auto token = lexer->tokens[i].get();
        auto found = lexer->resolved.find(i);
        auto resolved = found != lexer->resolved.end() ? lexer->tokens[found->second].get() : token;

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