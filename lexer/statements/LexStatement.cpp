// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 16/02/2024.
//

#include "lexer/Lexer.h"

bool Lexer::lexTopLevelStatementTokens() {
    return lexSingleLineCommentTokens() ||
           lexMultiLineCommentTokens() ||
           lexVarInitializationTokens(false) ||
           lexAnnotationMacro() ||
           (isLexImportStatement && lexImportStatement()) ||
           lexEnumStructureTokens() ||
           lexStructStructureTokens() ||
           lexTypealiasStatement() ||
           lexInterfaceStructureTokens() ||
           lexImplTokens() ||
           lexFunctionStructureTokens(true);
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