// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 16/02/2024.
//

#include "lexer/Lexer.h"

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

bool Lexer::hasNewLine() {
    return provider.peek() == '\n' || provider.peek() == '\r';
}

bool Lexer::lexNewLineChars() {
    auto peak = provider.peek();
    if (peak == '\n') {
        provider.readCharacter();
        return true;
    } else if (peak == '\r') {
        // consuming the \r
        provider.readCharacter();
        // consume also the next \n
        if (provider.peek() == '\n') provider.readCharacter();
        return true;
    } else {
        return false;
    }
}