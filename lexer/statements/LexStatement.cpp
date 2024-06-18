// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 16/02/2024.
//

#include "lexer/Lexer.h"
#include "cst/statements/ThrowCST.h"

bool Lexer::lexTopLevelStatementTokens() {
    return lexSingleLineCommentTokens() ||
           lexMultiLineCommentTokens() ||
           lexVarInitializationTokens(true) ||
           lexAnnotationMacro() ||
           lexEnumStructureTokens() ||
           lexStructStructureTokens() ||
           lexTypealiasStatement() ||
           lexInterfaceStructureTokens() ||
           lexImplTokens() ||
           lexFunctionStructureTokens(true, true);
}

bool Lexer::lexNestedLevelStatementTokens() {
    return lexSingleLineCommentTokens() ||
           lexMultiLineCommentTokens() ||
           lexVarInitializationTokens() ||
           lexAnnotationMacro() ||
           (isLexImportStatement && lexImportStatement()) ||
           (isLexBreakStatement && lexBreakStatement()) ||
           (isLexContinueStatement && lexContinueStatement()) ||
           (isLexReturnStatement && lexReturnStatement()) ||
           lexThrowStatementTokens() ||
           lexIfBlockTokens() ||
           lexTryCatchTokens() ||
           lexTypealiasStatement() ||
           lexSwitchStatementBlock() ||
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
           lexEnumStructureTokens() ||
           lexIfBlockTokens() ||
           lexForBlockTokens() ||
           lexStructStructureTokens() ||
           lexInterfaceStructureTokens() ||
           lexImplTokens() ||
           lexDoWhileBlockTokens() ||
           lexWhileBlockTokens() ||
           lexFunctionStructureTokens(true) ||
           lexAssignmentTokens();
}

bool Lexer::lexThrowStatementTokens() {
    if(lexKeywordToken("throw")) {
        auto start = tokens.size() - 1;
        lexWhitespaceToken();
        if(lexValueToken()) {
            error("expected a lambda value");
        } else {
            return false;
        }
        compound_from<ThrowCST>(start);
        return true;
    } else {
        return false;
    }
}