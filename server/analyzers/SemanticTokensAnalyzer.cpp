// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 11/03/2024.
//

#include "SemanticTokensAnalyzer.h"
#include "ast/base/ASTNode.h"
#include "lsp/types.h"
#include "utils/StringHelpers.h"
#include "compiler/cbi/model/CompilerBinder.h"
#include "server/cbi/hooks.h"

#define TokenType(e) (static_cast<uint32_t>(lsp::SemanticTokenTypes::e))

void SemanticTokensAnalyzer::put(
        uint32_t lineNumber,
        uint32_t lineCharNumber,
        uint32_t length,
        uint32_t tokenType,
        uint32_t tokenModifiers
) {
    const uint32_t lineDelta = lineNumber - prev_token_line_num;
    const uint32_t charDelta = (
            lineNumber == prev_token_line_num ? (
                    // on the same line
                    lineCharNumber - prev_token_char_num
            ) : (
                    // on a different line
                    lineCharNumber
            )
    );
    tokens.emplace_back(lineDelta);
    tokens.emplace_back(charDelta);
    tokens.emplace_back(length);
    tokens.emplace_back(tokenType);
    tokens.emplace_back(tokenModifiers);
    prev_token_char_num = lineCharNumber;
    prev_token_line_num = lineNumber;
}

void SemanticTokensAnalyzer::put(Token *token, uint32_t tokenType, uint32_t tokenModifiers) {
    put(token->position.line, token->position.character, token->value.size(), tokenType, tokenModifiers);
}

void SemanticTokensAnalyzer::put_node_token(Token* token, ASTNode* node) {
    switch (node->kind()) {
        case ASTNodeKind::FunctionParam:
        case ASTNodeKind::VariantMemberParam:
            put(token, TokenType(Parameter));
            return;
        case ASTNodeKind::GenericTypeParam:
            put(token, TokenType(TypeParameter));
            return;
        case ASTNodeKind::StructDecl:
        case ASTNodeKind::UnnamedStruct:
        case ASTNodeKind::UnionDecl:
        case ASTNodeKind::UnnamedUnion:
        case ASTNodeKind::VariantDecl:
            put(token, TokenType(Struct));
            return;
        case ASTNodeKind::EnumDecl:
            put(token, TokenType(Enum));
            return;
        case ASTNodeKind::EnumMember:
            put(token, TokenType(EnumMember));
            return;
        case ASTNodeKind::NamespaceDecl:
            put(token, TokenType(Namespace));
            return;
        case ASTNodeKind::InterfaceDecl:
            put(token, TokenType(Interface));
            return;
        case ASTNodeKind::FunctionDecl: {
            const auto parent = node->parent();
            if (parent) {
                const auto parent_kind = parent->kind();
                if (parent_kind == ASTNodeKind::VariantDecl || parent_kind == ASTNodeKind::StructDecl || parent_kind == ASTNodeKind::UnionDecl) {
                    put(token, TokenType(Method));
                    return;
                }
            }
            put(token, TokenType(Function));
            return;
        }
        case ASTNodeKind::VarInitStmt:
            put(token, TokenType(Variable));
            return;
        case ASTNodeKind::TypealiasStmt:
            put(token, TokenType(Type));
            return;
        default:
            put(token, TokenType(Variable));
            break;
    }
}

void SemanticTokensAnalyzer::put_auto(Token* token) {
    if(token->type >= TokenType::IndexKwStart && token->type <= TokenType::IndexKwEnd) {
        put(token, TokenType(Keyword));
    } else {
        switch (token->type) {
            case TokenType::EndOfFile:
            case TokenType::Unexpected:
            case TokenType::Whitespace:
            case TokenType::NewLine:
                break;
            case TokenType::Identifier: {
                ASTAny* anyPtr = token->linked;
                if(anyPtr) {
                    const auto anyKind = anyPtr->any_kind();
                    if(anyKind == ASTAnyKind::Node) {
                        put_node_token(token, (ASTNode*) anyPtr);
                        return;
                    }
                    const auto linked = anyPtr->get_ref_linked_node();
                    if(linked) {
                        put_node_token(token, linked);
                    } else {
                        put(token, TokenType(Variable));
                    }
                } else {
                    put(token, TokenType(Variable));
                }
                break;
            }
            case TokenType::LParen:
            case TokenType::RParen:
            case TokenType::LBrace:
            case TokenType::RBrace:
            case TokenType::LBracket:
            case TokenType::RBracket:
            case TokenType::PlusSym:
            case TokenType::MinusSym:
            case TokenType::MultiplySym:
            case TokenType::DivideSym:
            case TokenType::ModSym:
            case TokenType::DoublePlusSym:
            case TokenType::DoubleMinusSym:
            case TokenType::EqualSym:
            case TokenType::DoubleEqualSym:
            case TokenType::NotEqualSym:
            case TokenType::LessThanOrEqualSym:
            case TokenType::LessThanSym:
            case TokenType::GreaterThanOrEqualSym:
            case TokenType::GreaterThanSym:
            case TokenType::LogicalAndSym:
            case TokenType::LogicalOrSym:
            case TokenType::LeftShiftSym:
            case TokenType::RightShiftSym:
            case TokenType::AmpersandSym:
            case TokenType::PipeSym:
            case TokenType::CaretUpSym:
            case TokenType::BitNotSym:
            case TokenType::ColonSym:
            case TokenType::DoubleColonSym:
            case TokenType::NotSym:
            case TokenType::DotSym:
            case TokenType::CommaSym:
            case TokenType::SemiColonSym:
            case TokenType::TripleDotSym:
            case TokenType::LambdaSym:
                put(token, TokenType(Operator));
                break;
            case TokenType::String: {
                auto& pos = token->position;
                // +2 is added for the double quotes
                put(pos.line, pos.character, token->value.size() + 2, TokenType(String), 0);
                break;
            }
            case TokenType::MultilineString:
                // +3 for each """
                putMultilineToken(token, TokenType(String), 0, 3, 3);
                break;
            case TokenType::Char: {
                // +2 is added for the quotes
                auto& pos = token->position;
                put(pos.line, pos.character, token->value.size() + 2, TokenType(String), 0);
                break;
            }
            case TokenType::HashMacro: {

                // triggering nested semantic token put
                auto& t = *token;
                const auto view = chem::string_view(t.value.data() + 1, t.value.size() - 1);
                const auto hook = binder.findHook(view, CBIFunctionType::SemanticTokensPut);
                if(hook) {
                    ((EmbeddedSemanticTokensPut) hook)(this);
                } else {
                    put(token, TokenType(Macro));
                }

                break;
            }
            case TokenType::Annotation:
                put(token, TokenType(Macro));
                break;
            case TokenType::SingleLineComment: {
                auto& pos = token->position;
                // +2 for //
                put(pos.line, pos.character, token->value.size() + 2, TokenType(Comment), 0);
                break;
            }
            case TokenType::MultiLineComment:
                // +2 for each /*
                putMultilineToken(token, TokenType(Comment), 0, 2, 2);
                break;
            case TokenType::Number:
                put(token, TokenType(Number));
                break;
            default:
#ifdef DEBUG
                throw std::runtime_error("unhandled token type");
#else
                break;
#endif
        }
    }
}

void SemanticTokensAnalyzer::analyze(std::vector<Token>& lexedTokens) {
    current_token = lexedTokens.data();
    const auto endToken = current_token + lexedTokens.size();
    end_token = endToken;
    while(current_token != endToken) {
        put_auto(current_token);
        current_token++;
    }
}

/**
 * comment tokens must be divided for different lines
 * since vs code doesn't yet support a single multiline token
 */
void SemanticTokensAnalyzer::putMultilineToken(
        Token *token,
        uint32_t tokenType,
        uint32_t tokenModifiers,
        unsigned int charStartOffset,
        unsigned int charEndOffset
) {
    auto lineStart = token->position.line;
    auto charStart = token->position.character;
    // how many characters we have processed on current line
    int currLineChar = static_cast<int>(charStartOffset);
    auto& value = token->value;
    unsigned i = 0;
    const auto str_size = value.size();
    while(i < str_size) {
        const auto curr = value[i];
        if(curr == '\n') {
            put(lineStart, charStart, currLineChar, tokenType, tokenModifiers);
            currLineChar = -1;
            charStart = 0;
            lineStart++;
        } else if(curr == '\r' && (i + 1 < str_size && value[i + 1] == '\n')) {
            i++;
            continue;
        }
        currLineChar++;
        i++;
    }
    currLineChar += static_cast<int>(charEndOffset);
    if(currLineChar > 0) {
        put(lineStart, charStart, currLineChar, tokenType, tokenModifiers);
    }
};