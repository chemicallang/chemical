// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 11/03/2024.
//

#include "SemanticTokensAnalyzer.h"
#include <unordered_set>
#include "ast/base/ASTNode.h"

#define DEBUG false

void SemanticTokensAnalyzer::put(
        unsigned int lineNumber,
        unsigned int lineCharNumber,
        unsigned int length,
        unsigned int tokenType,
        unsigned int tokenModifiers
) {
    tokens.emplace_back(
            lineNumber - prev_token_line_num, (
                    lineNumber == prev_token_line_num ? (
                            // on the same line
                            lineCharNumber - prev_token_char_num
                    ) : (
                            // on a different line
                            lineCharNumber
                    )
            ), length, tokenType, tokenModifiers
    );
    prev_token_char_num = lineCharNumber;
    prev_token_line_num = lineNumber;
}

void SemanticTokensAnalyzer::put(Token *token, unsigned int tokenType, unsigned int tokenModifiers) {
    put(token->position.line, token->position.character, token->value.size(), tokenType, tokenModifiers);
}

void SemanticTokensAnalyzer::put_auto(Token* token) {
    if(token->type >= TokenType::IndexKwStart && token->type <= TokenType::IndexKwEnd) {
        put(token, SemanticTokenType::ls_keyword);
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
                                put(token, SemanticTokenType::ls_parameter);
                                return;
                            case ASTNodeKind::GenericTypeParam:
                                put(token, SemanticTokenType::ls_typeParameter);
                                return;
                            case ASTNodeKind::StructDecl:
                            case ASTNodeKind::UnnamedStruct:
                            case ASTNodeKind::UnionDecl:
                            case ASTNodeKind::UnnamedUnion:
                            case ASTNodeKind::VariantDecl:
                                put(token, SemanticTokenType::ls_struct);
                                return;
                            case ASTNodeKind::EnumDecl:
                                put(token, SemanticTokenType::ls_enum);
                                return;
                            case ASTNodeKind::EnumMember:
                                put(token, SemanticTokenType::ls_enumMember);
                                return;
                            case ASTNodeKind::NamespaceDecl:
                                put(token, SemanticTokenType::ls_namespace);
                                return;
                            case ASTNodeKind::InterfaceDecl:
                                put(token, SemanticTokenType::ls_interface);
                                return;
                            case ASTNodeKind::FunctionDecl: {
                                const auto parent = linked->parent();
                                if (parent) {
                                    const auto parent_kind = parent->kind();
                                    if (parent_kind == ASTNodeKind::VariantDecl || parent_kind == ASTNodeKind::StructDecl || parent_kind == ASTNodeKind::UnionDecl) {
                                        put(token, SemanticTokenType::ls_method);
                                        return;
                                    }
                                }
                                put(token, SemanticTokenType::ls_function);
                                return;
                            }
                            case ASTNodeKind::VarInitStmt:
                                put(token, SemanticTokenType::ls_variable);
                                return;
                            case ASTNodeKind::TypealiasStmt:
                                put(token, SemanticTokenType::ls_type);
                                return;
                            default:
                                put(token, SemanticTokenType::ls_variable);
                                break;
                        }
                    } else {
                        put(token, SemanticTokenType::ls_variable);
                    }
                } else {
                    put(token, SemanticTokenType::ls_variable);
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
                put(token, SemanticTokenType::ls_operator);
                break;
            case TokenType::String:
            case TokenType::Char:
                put(token, SemanticTokenType::ls_string);
                break;
            case TokenType::HashMacro:
            case TokenType::Annotation:
                put(token, SemanticTokenType::ls_macro);
                break;
            case TokenType::SingleLineComment:
                put(token, SemanticTokenType::ls_comment);
            case TokenType::MultiLineComment:
                putMultilineComment(token);
                break;
            case TokenType::Number:
                put(token, SemanticTokenType::ls_number);
                break;
            default:
#ifdef DEBUG
                throw std::runtime_error("unhandled token type");
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
void SemanticTokensAnalyzer::putMultilineComment(Token *token) {
    auto lineStart = token->position.line;
    auto charStart = token->position.character;
    const auto& val = token->value;
    const auto total_length = val.size();
    unsigned lengthCovered = 0;
    unsigned i = 0;
    while(i < total_length) {
        if(val[i] == '\n') {
            // comment from previous start
            put(lineStart, charStart, i - lengthCovered, SemanticTokenType::ls_comment, 0);
            // update the next token start
            lineStart++;
            charStart = 0;
            lengthCovered = i;
        } else if(val[i] == '\r') {
            // comment from previous start
            put(lineStart, charStart, i - lengthCovered, SemanticTokenType::ls_comment, 0);
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
        put(lineStart, charStart, total_length - lengthCovered, SemanticTokenType::ls_comment, 0);
    }
};