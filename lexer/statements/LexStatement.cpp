// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 16/02/2024.
//

#include "lexer/Lexer.h"

bool Lexer::lexTopLevelAccessSpecifiedDecls() {
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

bool Lexer::lexTopLevelStatementTokens() {
    return lexSingleLineCommentTokens() ||
           lexMultiLineCommentTokens() ||
           lexVarInitializationTokens(true) ||
           lexAnnotationMacro() ||
           lexUsingStatement() ||
           lexProvideStatement() ||
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

bool Lexer::lexNestedLevelStatementTokens(bool is_value, bool lex_value_node) {
    return lexSingleLineCommentTokens() ||
           lexMultiLineCommentTokens() ||
           lexVarInitializationTokens() ||
           lexAnnotationMacro() ||
           (isLexImportStatement && lexImportStatement()) ||
           (isLexInitBlock && lexConstructorInitBlock()) ||
           lexUnsafeBlock() ||
           (lexBreakStatement()) ||
           (lexContinueStatement()) ||
           (isLexReturnStatement && lexReturnStatement()) ||
           (lexDestructStatement()) ||
           lexThrowStatementTokens() ||
           lexUsingStatement() ||
           lexProvideStatement() ||
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

bool Lexer::lexStatementTokens() {
    return lexSingleLineCommentTokens() ||
           lexMultiLineCommentTokens() ||
           lexVarInitializationTokens() ||
           lexAnnotationMacro() ||
           (isLexImportStatement && lexImportStatement()) ||
           (lexBreakStatement()) ||
           (lexContinueStatement()) ||
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

bool Lexer::lexProvideStatement() {
    if(lexWSKeywordToken("provide")) {
        unsigned start = tokens_size() - 1;
        if(!lexAccessChainOrAddrOf()) {
            error("expected a value after provide keyword");
            return true;
        }
        if(!lexWSKeywordToken("as")) {
            error("expected 'as' keyword after the value ein provide statement");
            return true;
        }
        if(!lexIdentifierToken()) {
            error("expected a identifier after 'as' in provide statement");
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