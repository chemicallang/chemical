// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 16/02/2024.
//

#include "parser/Parser.h"

bool Parser::lexTopLevelAccessSpecifiedDecls() {
    if(lexAccessSpecifier(false)) {
        auto start = tokens_size() - 1;
        const auto found = lexFunctionStructureTokens(start, true, true)
                || lexEnumStructureTokens(start)
                || lexStructStructureTokens(start, false, false)
                || lexUnionStructureTokens(start, false, false)
                || lexVariantStructureTokens(start)
                || lexVarInitializationTokens(start, true, false)
                || lexInterfaceStructureTokens(start)
                || lexTypealiasStatement(start)
                || lexNamespaceTokens(start);
        if(!found) {
           error("expected a top level declaration after access specifier");
        }
        return true;
    } else {
        return false;
    }
}

bool Parser::lexTopLevelStatementTokens() {
    return lexSingleLineCommentTokens() ||
           lexMultiLineCommentTokens() ||
           lexVarInitializationTokens(true) ||
           lexAnnotationMacro() ||
           lexUsingStatement() ||
           lexProvideStatement() ||
           lexComptimeBlock() ||
           lexEnumStructureTokens() ||
           lexStructStructureTokens() ||
           lexUnionStructureTokens() ||
           lexVariantStructureTokens() ||
           lexTypealiasStatement() ||
           lexInterfaceStructureTokens() ||
           lexImplTokens() ||
           lexIfBlockTokens(false, false, true) ||
           lexFunctionStructureTokens(true, true) ||
           lexNamespaceTokens() ||
           lexTopLevelAccessSpecifiedDecls();
}

bool Parser::lexNestedLevelStatementTokens(bool is_value, bool lex_value_node) {
    return lexSingleLineCommentTokens() ||
           lexMultiLineCommentTokens() ||
           lexVarInitializationTokens() ||
           lexAnnotationMacro() ||
           (isLexImportStatement && lexImportStatement()) ||
           (isLexInitBlock && lexConstructorInitBlock()) ||
           lexUnsafeBlock() ||
           (lexBreakStatement()) ||
           (lexContinueStatement()) ||
           lexUnreachableStatement() ||
           (isLexReturnStatement && lexReturnStatement()) ||
           (lexDestructStatement()) ||
           lexThrowStatementTokens() ||
           lexUsingStatement() ||
           lexProvideStatement() ||
           lexComptimeBlock() ||
           lexIfBlockTokens(is_value, lex_value_node, false) ||
           lexTryCatchTokens() ||
           lexTypealiasStatement() ||
           lexSwitchStatementBlock(is_value, lex_value_node) ||
           lexLoopBlockTokens(is_value) ||
           lexForBlockTokens() ||
           lexDoWhileBlockTokens() ||
           lexWhileBlockTokens() ||
           lexAssignmentTokens();
}

bool Parser::lexStatementTokens() {
    return lexSingleLineCommentTokens() ||
           lexMultiLineCommentTokens() ||
           lexVarInitializationTokens() ||
           lexAnnotationMacro() ||
           (isLexImportStatement && lexImportStatement()) ||
           (lexBreakStatement()) ||
           (lexContinueStatement()) ||
           lexUnreachableStatement() ||
           (isLexReturnStatement && lexReturnStatement()) ||
           (lexDestructStatement()) ||
           lexEnumStructureTokens() ||
           lexIfBlockTokens(false, false, false) ||
           lexLoopBlockTokens(false) ||
           lexForBlockTokens() ||
           lexStructStructureTokens() ||
           lexUnionStructureTokens() ||
           lexInterfaceStructureTokens() ||
           lexImplTokens() ||
           lexDoWhileBlockTokens() ||
           lexWhileBlockTokens() ||
           lexFunctionStructureTokens(true) ||
           lexAssignmentTokens();
}

bool Parser::lexThrowStatementTokens() {
    if(lexWSKeywordToken(TokenType::ThrowKw)) {
        auto start = tokens_size() - 1;
        if(lexValueToken()) {
            error("expected a lambda value");
        } else {
            return false;
        }
        compound_from(start, LexTokenType::CompThrow);
        return true;
    } else {
        return false;
    }
}

bool Parser::lexUsingStatement() {
    if(lexWSKeywordToken(TokenType::UsingKw)) {
        auto start = tokens_size() - 1;
        lexWSKeywordToken(TokenType::NamespaceKw);
        do {
            if(!lexIdentifierToken()) {
                error("expected a identifier in using statement");
                return true;
            }
        } while(lexOperatorToken(TokenType::DoubleColonSym));
        compound_from(start, LexTokenType::CompUsing);
        return true;
    } else {
        return false;
    }
}

bool Parser::lexProvideStatement() {
    if(lexWSKeywordToken(TokenType::ProvideKw)) {
        unsigned start = tokens_size() - 1;
        if(!lexExpressionTokens()) {
            error("expected a value after provide keyword");
            return true;
        }
        if(!lexBraceBlock("provide")) {
            mal_node(start, "missing body for provide statement");
            return true;
        }
        compound_from(start, LexTokenType::CompProvide);
    } else {
        return false;
    }
}

bool Parser::lexComptimeBlock() {
    if(lexWSKeywordToken(TokenType::ComptimeKw, TokenType::LBrace)) {
        unsigned start = tokens_size() - 1;
        if(!lexBraceBlock("comptime")) {
            mal_node(start, "missing body for provide statement");
            return true;
        }
        compound_from(start, LexTokenType::CompComptime);
    } else {
        return false;
    }
}