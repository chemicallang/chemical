// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 16/02/2024.
//

#include "lexer/Lexer.h"

bool Lexer::lexTopLevelStatementTokens() {
    return lexSingleLineCommentTokens() ||
           lexMultiLineCommentTokens() ||
           lexVarInitializationTokens(true) ||
           lexAnnotationMacro() ||
           lexUsingStatement() ||
           lexEnumStructureTokens() ||
           lexStructStructureTokens() ||
           lexUnionStructureTokens() ||
           lexVariantStructureTokens() ||
           lexTypealiasStatement() ||
           lexInterfaceStructureTokens() ||
           lexImplTokens() ||
           lexFunctionStructureTokens(true, true) ||
           lexNamespaceTokens();
}

bool Lexer::lexNestedLevelStatementTokens(bool is_value, bool lex_value_node) {
    return lexSingleLineCommentTokens() ||
           lexMultiLineCommentTokens() ||
           lexVarInitializationTokens() ||
           lexAnnotationMacro() ||
           (isLexImportStatement && lexImportStatement()) ||
           (isLexBreakStatement && lexBreakStatement()) ||
           (isLexContinueStatement && lexContinueStatement()) ||
           (isLexReturnStatement && lexReturnStatement()) ||
           (isLexDestructStatement && lexDestructStatement()) ||
           lexThrowStatementTokens() ||
           lexUsingStatement() ||
           lexIfBlockTokens(is_value, lex_value_node) ||
           lexTryCatchTokens() ||
           lexTypealiasStatement() ||
           lexSwitchStatementBlock(is_value, lex_value_node) ||
           lexLoopBlockTokens(is_value) ||
           lexForBlockTokens() ||
           lexDoWhileBlockTokens() ||
           lexWhileBlockTokens() ||
           lexAssignmentTokens();
}

bool Lexer::lexStatementTokens() {
    return lexSingleLineCommentTokens() ||
           lexMultiLineCommentTokens() ||
           lexVarInitializationTokens() ||
           lexAnnotationMacro() ||
           (isLexImportStatement && lexImportStatement()) ||
           (isLexBreakStatement && lexBreakStatement()) ||
           (isLexContinueStatement && lexContinueStatement()) ||
           (isLexReturnStatement && lexReturnStatement()) ||
           (isLexDestructStatement && lexDestructStatement()) ||
           lexEnumStructureTokens() ||
           lexIfBlockTokens(false, false) ||
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

bool Lexer::lexThrowStatementTokens() {
    if(lexWSKeywordToken("throw")) {
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

bool Lexer::lexUsingStatement() {
    if(lexWSKeywordToken("using")) {
        auto start = tokens_size() - 1;
        lexWSKeywordToken("namespace");
        do {
            if(!lexIdentifierToken()) {
                error("expected a identifier in using statement");
                return true;
            }
        } while(lexOperatorToken("::"));
        compound_from(start, LexTokenType::CompUsing);
        return true;
    } else {
        return false;
    }
}