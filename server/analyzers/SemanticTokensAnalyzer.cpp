// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 11/03/2024.
//

#include "SemanticTokensAnalyzer.h"
#include "ast/base/ASTNode.h"
#include "lsp/types.h"

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
                // TODO calculate the anyptr for this token
                ASTAny* anyPtr = nullptr;
                if(anyPtr) {
                    const auto linked = anyPtr->get_ref_linked_node();
                    if(linked) {
                        const auto linked_kind = linked->kind();
                        switch (linked_kind) {
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
                                const auto parent = linked->parent();
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
            case TokenType::String:
                putMultilineToken(token, TokenType(String));
                break;
            case TokenType::Char:
                put(token, TokenType(String));
                break;
            case TokenType::HashMacro:
            case TokenType::Annotation:
                put(token, TokenType(Macro));
                break;
            case TokenType::SingleLineComment:
                put(token, TokenType(Comment));
                break;
            case TokenType::MultiLineComment:
                putMultilineToken(token, TokenType(Comment));
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

void SemanticTokensAnalyzer::visit(std::vector<Token> &tokens_vec, unsigned start, unsigned till) {
    auto i = start;
    while (i < till) {
        put_auto(&tokens_vec[i]);
        i++;
    }
}

/**
 * comment tokens must be divided for different lines
 * since vs code doesn't yet support a single multiline token
 */
void SemanticTokensAnalyzer::putMultilineToken(Token *token, uint32_t tokenType, uint32_t tokenModifiers) {
    auto lineStart = token->position.line;
    auto charStart = token->position.character;
    const auto& val = token->value;
    const auto total_length = val.size();
    unsigned lengthCovered = 0;
    unsigned i = 0;
    while(i < total_length) {
        if(val[i] == '\n') {
            // comment from previous start
            put(lineStart, charStart, i - lengthCovered, tokenType, tokenModifiers);
            // update the next token start
            lineStart++;
            charStart = 0;
            lengthCovered = i;
        } else if(val[i] == '\r') {
            // comment from previous start
            put(lineStart, charStart, i - lengthCovered, tokenType, tokenModifiers);
            // consume the next line ending
            if(val[i + 1] == '\n') {
                i++;
            }
            // update the next token start
            lineStart++;
            charStart = 0;
            lengthCovered = i;
        }
        i++;
    }
    if(lengthCovered < total_length) {
        put(lineStart, charStart, total_length - lengthCovered, tokenType, tokenModifiers);
    }
};