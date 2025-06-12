// Copyright (c) Chemical Language Foundation 2025.

#include "GotoDefAnalyzer.h"
#include "server/model/LexResult.h"
#include <iostream>
#include "utils/PathUtils.h"
#include "ast/base/ASTNode.h"
#include "core/source/LocationManager.h"
#include "server/utils/AnalyzerUtils.h"

#ifdef DEBUG
#define DEBUG_GOTO_DEF
#endif

GotoDefAnalyzer::GotoDefAnalyzer(
    LocationManager& manager,
    Position position
) : manager(manager), position(position) {
    // do nothing
}

std::vector<lsp::DefinitionLink> GotoDefAnalyzer::analyze(std::vector<Token>& tokens) {
    std::vector<lsp::DefinitionLink> defs;
    auto token = get_token_at_position(tokens, position);
    if(token) {
        const auto astAny = token->linked;
        if(astAny) {
            const auto ref_linked = astAny->get_ref_linked_node();
            if (ref_linked) {
                const auto encoded = ref_linked->encoded_location();
                if(encoded.isValid()) {
                    const auto location = manager.getLocationPos(encoded);
                    const auto filePath = manager.getPathForFileId(location.fileId);
                    defs.emplace_back(
                            lsp::DocumentUri::fromPath(filePath),
                            lsp::Range(
                                    lsp::Position(location.start.line, location.start.character),
                                    lsp::Position(location.end.line, location.end.character)
                            ),
                            lsp::Range(
                                    lsp::Position(location.start.line, location.start.character),
                                    lsp::Position(location.end.line, location.end.character)
                            )
                    );
                } else {
#ifdef DEBUG_GOTO_DEF
                    std::cout << "[GotoDefAnalyzer] invalid location preset " << position.representation();
#endif
                }
            } else {
#ifdef DEBUG_GOTO_DEF
                std::cout << "[GotoDefAnalyzer] linked empty " << position.representation();
#endif
            }
        } else {
#ifdef DEBUG_GOTO_DEF
            std::cout << "[GotoDefAnalyzer] token not linked " << position.representation();
#endif
        }
    } else {
#ifdef DEBUG_GOTO_DEF
        std::cout << "[GotoDefAnalyzer] token not found" << position.representation();
#endif
    }
    return std::move(defs);
}