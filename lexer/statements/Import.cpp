// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "lexer/Lexer.h"

bool Lexer::lexImportStatement() {
    if(lexKeywordToken("import")) {
        lexWhitespaceToken();
        if(!lexStringToken()) {
            error("path is required after the import statement");
        }
        return true;
    } else{
        return false;
    }
}