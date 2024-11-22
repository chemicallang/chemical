// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "parser/Parser.h"

void Parser::lexUnionBlockTokens() {
    do {
        lexWhitespaceAndNewLines();
        if(!(
            lexStructMemberTokens() ||
            lexFunctionStructureTokens() ||
            lexSingleLineCommentTokens() ||
            lexMultiLineCommentTokens() ||
            lexStructStructureTokens(true, true) ||
            lexAnnotationMacro()
        )) {
            break;
        }
        lexWhitespaceToken();
        lexOperatorToken(';');
    } while(provider.peek() != '}');
    lexWhitespaceToken();
}

bool Parser::lexUnionStructureTokens(unsigned start_token, bool unnamed, bool direct_init) {
    if(lexWSKeywordToken("union")) {
        bool has_identifier = false;
        if(!unnamed) {
            has_identifier = lexIdentifierToken();
            if (!has_identifier) {
                error("expected a identifier as union name");
                return true;
            }
        }
        lexWhitespaceToken();
        if(!lexOperatorToken('{')) {
            error("expected a '{' for union block");
            return true;
        }
        lexUnionBlockTokens();
        if(!lexOperatorToken('}')) {
            error("expected a closing bracket '}' for union block");
            return true;
        }
        if(lexWhitespaceToken() && !has_identifier && direct_init && !lexIdentifierToken()) {
            error("expected an identifier after the '}' for anonymous union definition");
            return true;
        }
        compound_from(start_token, LexTokenType::CompUnionDef);
        return true;
    } else {
        return false;
    }
}