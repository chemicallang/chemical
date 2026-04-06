// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 11/03/2024.
//

#include "SemanticTokensAnalyzer.h"
#include "ast/base/ASTNode.h"
#include "ast/statements/VarInit.h"
#include "lsp/types.h"

#include "utils/StringHelpers.h"
#include "compiler/cbi/model/CompilerBinder.h"
#include "server/cbi/hooks.h"
#include "std/except.h"
#include "ast/base/ASTNodeKind.h"
#include "ast/base/ASTAnyKind.h"

#include "server/model/SemanticTokenScopes.h"


#define TokenType(e) (static_cast<uint32_t>(lsp::SemanticTokenTypes::e))
#define TokenScope(e) (static_cast<uint32_t>(SemanticTokenScopes::e))


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
            put(token, TokenScope(VariableParameterFunction));
            return;
        case ASTNodeKind::GenericTypeParam:
            put(token, TokenScope(EntityNameType));
            return;
        case ASTNodeKind::StructDecl:
        case ASTNodeKind::UnnamedStruct:
        case ASTNodeKind::UnionDecl:
        case ASTNodeKind::UnnamedUnion:
        case ASTNodeKind::VariantDecl:
            put(token, TokenScope(EntityNameTypeClass));
            return;
        case ASTNodeKind::EnumDecl:
            put(token, TokenScope(EntityNameType));
            return;
        case ASTNodeKind::EnumMember:
            put(token, TokenScope(VariableOtherEnummember));
            return;
        case ASTNodeKind::NamespaceDecl:
            put(token, TokenScope(EntityNameNamespace));
            return;
        case ASTNodeKind::InterfaceDecl:
            put(token, TokenScope(EntityNameType));
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
            put(token, TokenScope(EntityNameFunction));
            return;
        }
        case ASTNodeKind::VarInitStmt: {
            auto varInit = node->as_var_init_unsafe();
            if (varInit->is_const()) {
                put(token, TokenScope(VariableOtherConstant));
            } else {
                put(token, TokenScope(EntityNameVariable));
            }
            return;
        }
        case ASTNodeKind::TypealiasStmt:
            put(token, TokenScope(EntityNameType));
            return;
        default:
            put(token, TokenType(Variable));
            break;
    }
}


Token* SemanticTokensAnalyzer::put_auto(Token* token) {
    if(token->type >= TokenType::IndexKwStart && token->type <= TokenType::IndexKwEnd) {
        switch (token->type) {
            case TokenType::ForKw:
            case TokenType::SwitchKw:
            case TokenType::LoopKw:
            case TokenType::ReturnKw:
            case TokenType::BreakKw:
            case TokenType::ContinueKw:
            case TokenType::IfKw:
            case TokenType::ElseKw:
            case TokenType::WhileKw:
            case TokenType::DoKw:
            case TokenType::TryKw:
            case TokenType::CatchKw:
            case TokenType::ThrowKw:
                put(token, TokenScope(KeywordControl));
                break;
            case TokenType::FuncKw:
            case TokenType::VarKw:
            case TokenType::TypeKw:
            case TokenType::StructKw:
            case TokenType::UnionKw:
            case TokenType::VariantKw:
            case TokenType::InterfaceKw:
            case TokenType::EnumKw:
            case TokenType::AliasKw:
            case TokenType::NamespaceKw:
            case TokenType::UsingKw:
            case TokenType::ImplKw:
                put(token, TokenScope(StorageType));
                break;
            case TokenType::PublicKw:
            case TokenType::PrivateKw:
            case TokenType::ProtectedKw:
            case TokenType::InternalKw:
            case TokenType::ExportKw:
            case TokenType::UnsafeKw:
            case TokenType::ComptimeKw:
            case TokenType::DynKw:
            case TokenType::ConstKw:
            case TokenType::MutKw:
            case TokenType::UnreachableKw:
            case TokenType::ProvideKw:
            case TokenType::DefaultKw:
                put(token, TokenScope(StorageModifier));
                break;
            case TokenType::I8Kw: case TokenType::I16Kw: case TokenType::I32Kw: case TokenType::I64Kw:
            case TokenType::U8Kw: case TokenType::U16Kw: case TokenType::U32Kw: case TokenType::U64Kw:
            case TokenType::CharKw: case TokenType::ShortKw: case TokenType::IntKw: case TokenType::LongKw:
            case TokenType::LongLongKw: case TokenType::BigIntKw: case TokenType::Int128Kw:
            case TokenType::UCharKw: case TokenType::UShortKw: case TokenType::UIntKw: case TokenType::ULongKw:
            case TokenType::ULongLongKw: case TokenType::UBigIntKw: case TokenType::UInt128Kw:
            case TokenType::BoolKw:
            case TokenType::AnyKw:
            case TokenType::DoubleKw:
            case TokenType::LongdoubleKw:
            case TokenType::FloatKw:
            case TokenType::Float128Kw:
            case TokenType::VoidKw:
                put(token, TokenScope(SupportTypePrimitive));
                break;
            case TokenType::TrueKw:
            case TokenType::FalseKw:
            case TokenType::NullKw:
                put(token, TokenScope(ConstantLanguage));
                break;
            case TokenType::ThisKw:
                put(token, TokenScope(VariableLanguageThis));
                break;
            case TokenType::SelfKw:
                put(token, TokenScope(VariableLanguage));
                break;
            case TokenType::AsKw:
            case TokenType::IsKw:
            case TokenType::InKw:
            case TokenType::NewKw:
            case TokenType::DeleteKw:
            case TokenType::DeallocKw:
            case TokenType::SizeOfKw:
            case TokenType::AlignOfKw:
            case TokenType::FromKw:
            case TokenType::ZeroedKw:
                put(token, TokenScope(KeywordOperator));
                break;
            default:
                put(token, TokenType(Keyword));
                break;
        }
        return token + 1;
    }
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
                    return token + 1;
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
            put(token, TokenScope(PunctuationSectionParensBeginBracketRoundC));
            break;
        case TokenType::RParen:
            put(token, TokenScope(PunctuationSectionParensEndBracketRoundC));
            break;
        case TokenType::LBrace:
            put(token, TokenScope(PunctuationSectionBlockBeginBracketCurlyC));
            break;
        case TokenType::RBrace:
            put(token, TokenScope(PunctuationSectionBlockEndBracketCurlyC));
            break;
        case TokenType::LBracket:
            put(token, TokenScope(PunctuationSectionArrayBeginPhp));
            break;
        case TokenType::RBracket:
            put(token, TokenScope(PunctuationSectionArrayEndPhp));
            break;
        case TokenType::PlusSym:
        case TokenType::MinusSym:
        case TokenType::MultiplySym:
        case TokenType::DivideSym:
        case TokenType::ModSym:
        case TokenType::DoublePlusSym:
        case TokenType::DoubleMinusSym:
            put(token, TokenScope(KeywordOperatorArithmetic));
            break;
        case TokenType::EqualSym:
            put(token, TokenScope(KeywordOperatorAssignment));
            break;
        case TokenType::DoubleEqualSym:
        case TokenType::NotEqualSym:
        case TokenType::LessThanOrEqualSym:
        case TokenType::LessThanSym:
        case TokenType::GreaterThanOrEqualSym:
        case TokenType::GreaterThanSym:
            put(token, TokenScope(KeywordOperatorComparison));
            break;
        case TokenType::LogicalAndSym:
        case TokenType::LogicalOrSym:
        case TokenType::NotSym:
            put(token, TokenScope(KeywordOperatorLogical));
            break;
        case TokenType::LeftShiftSym:
        case TokenType::RightShiftSym:
        case TokenType::AmpersandSym:
        case TokenType::PipeSym:
        case TokenType::CaretUpSym:
        case TokenType::BitNotSym:
            put(token, TokenScope(KeywordOperatorBitwise));
            break;
        case TokenType::ColonSym:
        case TokenType::DotSym:
        case TokenType::CommaSym:
        case TokenType::SemiColonSym:
        case TokenType::TripleDotSym:
            put(token, TokenScope(PunctuationSeparatorDelimiter));
            break;
        case TokenType::DoubleColonSym:
            put(token, TokenScope(EntityNameScopeResolution));
            break;
        case TokenType::DollarSym:
            put(token, TokenScope(PunctuationSeparatorDelimiter));
            break;
        case TokenType::LambdaSym:
            put(token, TokenScope(KeywordOperator));
            break;
        case TokenType::QuestionMarkSym:
            put(token, TokenScope(KeywordOperatorTernary));
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
                return ((EmbeddedSemanticTokensPut) hook)(this, &t, end_token);
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
    case TokenType::BacktickString:
            put(token, TokenType(String));
            break;
    case TokenType::StringExprStart:
            // ${
            put(token, TokenType(Operator));
            break;
    case TokenType::StringExprEnd:
            if (token->value.size() <= 1) {
                put(token, TokenType(Operator));
            } else {
                // first we put '}' (rbrace), then rest is a string
                put(token->position.line, token->position.character, 1, TokenType(Operator), 0);
                put(token->position.line, token->position.character + 1, token->value.size() - 1, TokenType(String), 0);
            }
            break;
        default:
            put(token, TokenType(Comment));
            break;
    }
    return token + 1;
}

void SemanticTokensAnalyzer::analyze(std::vector<Token>& lexedTokens) {
    // for each token we have 5 integers to hold
    tokens.reserve(lexedTokens.size() * 5);
    auto current = lexedTokens.data();
    const auto endToken = lexedTokens.data() + lexedTokens.size();
    end_token = endToken;
    while(current != endToken) {
        current = put_auto(current);
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