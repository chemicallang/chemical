// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/03/2024.
//

#include "parser/utils/AnnotationModifiers.h"
#include "parser/utils/MacroLexers.h"
#include "parser/model/CompilerBinder.h"

bool Parser::lexAnnotationMacro() {

    if (!(token->type == TokenType::AtSym || token->type == TokenType::HashSym)) {
        return false;
    }

    auto isAnnotation = token->type == TokenType::AtSym;
    auto& bare_start_pos = token->position;
    token++;
    auto& start_pos = token->position;
    chem::string macro_full_chem;
    macro_full_chem.append(isAnnotation ? '@' : '#');
    while(true) {
        auto& tok = *token;
        auto tok_type = tok.type;
        if(tok_type == TokenType::Identifier || Parser::isKeyword(tok_type)) {
            macro_full_chem.append(tok.value);
            token++;
        } else if(tok_type == TokenType::DotSym) {
            // dots are allowed in macro or annotation names
            macro_full_chem.append('.');
            token++;
        } else {
            break;
        }
    }
    // macro name without the # or @ symbol
    auto macro_view = std::string_view { macro_full_chem.data() + 1, macro_full_chem.size() - 1 };
    auto macro = std::string(macro_view);

    // if it's annotation
    if (isAnnotation) {
        unsigned start = tokens_size();
        emplace(LexTokenType::Annotation, start_pos, macro_full_chem.to_std_string());
        if(lexOperatorToken(TokenType::LParen)) {
            do {
                lexWhitespaceToken();
                if(!lexExpressionTokens()) {
                    break;
                }
                lexWhitespaceToken();
            } while (lexOperatorToken(TokenType::CommaSym));
            if(!lexOperatorToken(TokenType::RParen)) {
                error("expected a ')' after '(' to call an annotation");
                return true;
            }
            compound_from(start, LexTokenType::CompAnnotation);
        }
        auto found = AnnotationModifiers.find(macro_view);
        if (found != AnnotationModifiers.end()) {
            found->second(this, unit.tokens[start]);
        }
        return true;
    }

    auto start = tokens_size();

    emplace(LexTokenType::Identifier, start_pos, macro_full_chem.to_std_string());

    lexWhitespaceToken();
    if (lexOperatorToken(TokenType::LBrace)) {

        lexWhitespaceAndNewLines();

        // check if this macro has a lexer defined
        auto macro_lexer = MacroHandlers.find(macro);
        if (macro_lexer != MacroHandlers.end()) {
            macro_lexer->second(this);
        } else {
            auto lex_func = binder->provide_lex_macro_func(macro);
            if(lex_func) {
                lex_func(this);
            } else {
                error("couldn't find lexMacro function in cbi '" + macro + "', make sure the function is public");
                return true;
            }
        }

    } else {
        error("expected '{' after the macro " + macro);
        return true;
    }

    lexWhitespaceAndNewLines();
    if (!lexOperatorToken(TokenType::RBrace)) {
        error("expected '}' for the macro " + macro + " ending");
        return true;
    }

    compound_from(start, LexTokenType::CompMacro);
    return true;

}