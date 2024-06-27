// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 2/19/2024.
//

#include "lexer/Lexer.h"
#include "lexer/model/tokens/TypeToken.h"
#include "lexer/model/tokens/VariableToken.h"
#include "cst/types/FunctionTypeCST.h"
#include "cst/types/ReferencedValueTypeCST.h"
#include "cst/values/AccessChainCST.h"
#include "cst/types/ArrayTypeCST.h"
#include "cst/types/PointerTypeCST.h"
#include "cst/types/GenericTypeCST.h"

bool Lexer::lexLambdaTypeTokens(unsigned int start) {
    if(lexOperatorToken('(')) {
        lexParameterList();
        if(!lexOperatorToken(')')) {
            error("expected a ')' after the ')' in lambda function type");
        }
        lexWhitespaceToken();
        if(lexOperatorToken("=>")) {
            lexWhitespaceToken();
            if(!lexTypeTokens()) {
                error("expected a return type for lambda function type");
            }
        } else {
            error("expected '=>' for lambda function type");
        }
        compound_from<FunctionTypeCST>(start);
        return true;
    } else {
        return false;
    }
}

bool Lexer::lexTypeTokens() {

    if(lexOperatorToken('[')) {
        unsigned start = tokens.size() - 1;
        if(!lexOperatorToken(']')) {
            error("expected ']' after '[' for lambda type");
            return true;
        }
        lexWhitespaceToken();
        if(!lexLambdaTypeTokens(start)) {
            error("expected a lambda type after '[]'");
        }
        return true;
    }

    if(lexLambdaTypeTokens(tokens.size())) {
        return true;
    }

    std::string type = provider.readIdentifier();
    if(type.empty()) return false;
    bool has_multiple = false;
    unsigned start = tokens.size();
    while(true) {
        if(provider.peek() == ':' && provider.peek(1) == ':') {
            tokens.emplace_back(std::make_unique<VariableToken>(backPosition(type.length()), type));
            lexOperatorToken("::");
            auto new_type = provider.readIdentifier();
            if(new_type.empty()) {
                error("expected an identifier after '" + type + "::' for a type");
                return true;
            } else {
                has_multiple = true;
                type = new_type;
            }
        } else {
            if(has_multiple) {
                tokens.emplace_back(std::make_unique<VariableToken>(backPosition(type.length()), type));
                compound_from<AccessChainCST>(start);
                compound_from<ReferencedValueTypeCST>(start);
            } else {
                tokens.emplace_back(std::make_unique<TypeToken>(backPosition(type.length()), type));
            }
            break;
        }
    }

    if(lexOperatorToken('<')) {
        if(!lexTypeTokens()) {
            error("expected a type within '<' '>' for generic type");
        }
        if(!lexOperatorToken('>')) {
            error("expected '>' for generic type");
        }
        compound_from<GenericTypeCST>(start);
    } else if(lexOperatorToken('[')) {
        // optional array size
        lexUnsignedIntAsNumberToken();
        if(!lexOperatorToken(']')) {
            error("expected ']' for array type");
        }
        compound_from<ArrayTypeCST>(start);
    }
    while(lexOperatorToken('*')) {
        compound_from<PointerTypeCST>(start);
    }

    return true;

}