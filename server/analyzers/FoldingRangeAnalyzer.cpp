// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 11/03/2024.
//

#include "FoldingRangeAnalyzer.h"
#include "ast/structures/Scope.h"
#include "core/source/LocationManager.h"
#include "compiler/cbi/model/CompilerBinder.h"
#include "server/cbi/hooks.h"
#include "ast/base/Value.h"
#include "ast/base/ASTNode.h"
#include "ast/values/EmbeddedValue.h"
#include "ast/statements/EmbeddedNode.h"

void FoldingRangeAnalyzer::folding_range(const Position& start, const Position& end, bool comment) {
    ranges.push_back(lsp::FoldingRange{
            start.line,
            end.line,
            start.character,
            end.character,
            comment ? lsp::FoldingRangeKind::Comment : lsp::FoldingRangeKind::Region
    });
}

void* getDataPtr(Token& token) {
    const auto linked = token.linked;
    switch(linked->any_kind()) {
        case ASTAnyKind::Value: {
            const auto value = (Value*) linked;
            if(value->kind() == ValueKind::EmbeddedValue) {
                return value->as_embedded_value_unsafe()->data_ptr;
            } else {
                return nullptr;
            }
        }
        case ASTAnyKind::Node: {
            const auto node = (ASTNode*) linked;
            if (node->kind() == ASTNodeKind::EmbeddedNode) {
                return node->as_embedded_node_unsafe()->data_ptr;
            } else {
                return nullptr;
            }
        }
        default:
            return nullptr;
    }
}

void FoldingRangeAnalyzer::analyze(Token* current, Token* end) {
    while(current != end) {
        auto& token = *current;
        switch(token.type) {
            case TokenType::LBrace:
                bracesStack.emplace_back(token.position);
                break;
            case TokenType::RBrace:
                if(!bracesStack.empty()) {
                    const auto opening = bracesStack.back();
                    bracesStack.pop_back();
                    folding_range(opening, token.position, false);
                }
                break;
            case TokenType::HashMacro: {
                auto& t = token.value;
                auto view = chem::string_view(t.data() + 1, t.size() - 1);
                const auto hook = binder.findHook(view, CBIFunctionType::FoldingRangesPut);
                if (hook) {
                    current = ((EmbeddedFoldingRangesPut) hook)(this, current, end, getDataPtr(token));
                }
                break;
            }
            default:
                break;
        }
        current++;
    }
}

void FoldingRangeAnalyzer::analyze(std::vector<Token>& tokens) {
    ranges.reserve(120);
    bracesStack.reserve(20);
    auto current = tokens.data();
    const auto end = current + tokens.size();
    analyze(current, end);
}