// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/03/2024.
//

#include "LspSemanticTokens.h"
#include "SemanticLinker.h"
#include "utils/JsonUtils.h"
#include "utils/FileUtils.h"
#include "utils/Utils.h"
#include "LibLsp/lsp/AbsolutePath.h"
#include "LibLsp/lsp/textDocument/publishDiagnostics.h"

#define DEBUG false
#define OVER_SRC_PRINT false
#define PRINT_TOKENS false

std::vector<SemanticToken> to_semantic_tokens(FileTracker &tracker, const lsDocumentUri &uri, RemoteEndPoint &sp) {

    auto path = uri.GetAbsolutePath().path;

    auto overridden_source = tracker.get_overridden_source(path);

    std::vector<std::unique_ptr<LexToken>> lexed;

    std::vector<Diagnostic> errors;

    if (overridden_source.has_value()) {
        if(OVER_SRC_PRINT) std::cout << "[to_semantic_tokens] overridden source : " << overridden_source.value() << '\n';
        std::istringstream iss(overridden_source.value());
        StreamSourceProvider reader(iss);
        Lexer lexer(reader, path);
        lexer.lex();
        lexed = std::move(lexer.tokens);
        errors = std::move(lexer.errors);
    } else {
        if(OVER_SRC_PRINT) std::cout << "[to_semantic_tokens] overridden source not found for : " << path << '\n';
        std::ifstream file;
        file.open(path);
        if (!file.is_open()) {
            std::cerr << "Unknown error opening the file" << '\n';
        }
        StreamSourceProvider reader(file);
        Lexer lexer(reader, path);
        lexer.lex();
        lexed = std::move(lexer.tokens);
        errors = std::move(lexer.errors);
        file.close();
    }

    SemanticLinker linker(lexed);

    linker.analyze();

    // publishing diagnostics related to the lexing
    std::vector<lsDiagnostic> diagnostics;
    for(const auto &error : errors) {
        diagnostics.push_back(lsDiagnostic{
            lsRange(
                    lsPosition(error.range.start.line, error.range.start.character),
                    lsPosition(error.range.end.line, error.range.end.character)
                ),
                lsDiagnosticSeverity::Error,
                std::nullopt,
                std::nullopt,
                std::nullopt,
                error.message
        });
    }
    // adding diagnostics related to linking (unresolved references)
    for(const auto &not_found : linker.not_found) {
        auto token = lexed[not_found].get();
        diagnostics.push_back(lsDiagnostic{
                lsRange(
                        lsPosition(token->lineNumber(), token->lineCharNumber()),
                        lsPosition(token->lineNumber(), token->lineCharNumber() + token->length())
                ),
                lsDiagnosticSeverity::Error,
                std::nullopt,
                std::nullopt,
                std::nullopt,
                "Unresolved reference"
        });
    }
    Notify_TextDocumentPublishDiagnostics::notify notify;
    notify.params.uri = lsDocumentUri::FromPath(uri.GetAbsolutePath());
    notify.params.diagnostics = std::move(diagnostics);
    std::future<void> futureObj = std::async(std::launch::async, [&]{
        sp.sendNotification(notify);
    });

    if(PRINT_TOKENS) {
        printTokens(lexed, linker.resolved);
    }

    if(DEBUG) {
        auto overridden = tracker.get_overridden_source(path);
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