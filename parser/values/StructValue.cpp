// Copyright (c) Qinetik 2024.

#include "parser/Parser.h"
#include "ast/values/StructValue.h"
#include "ast/base/MalformedInput.h"

StructMemberInitializer* initializer(ASTAllocator& allocator, StructValue* struct_value, Token* id, Value* value) {
    return new (allocator.allocate<StructMemberInitializer>()) StructMemberInitializer(std::string(id->value), value, struct_value);
}

StructValue* Parser::parseStructValue(ASTAllocator& allocator, BaseType* refType, Position& start) {

    if(consumeToken(TokenType::LBrace)) {

        auto structValue = new (allocator.allocate<StructValue>()) StructValue(refType, { }, nullptr, 0, parent_node);

        // lex struct member value tokens
        do {
            lexWhitespaceAndNewLines();
            auto id = consumeIdentifierOrKeyword();
            if(id) {
                readWhitespace();
                if(!consumeToken(TokenType::ColonSym)) {
                    error("expected a ':' for initializing struct member " + std::string(id->value));
                    return structValue;
                }
                readWhitespace();
                auto expression = parseExpression(allocator, true);
                if(expression) {
                    structValue->values[std::string(id->value)] = initializer(allocator, structValue, id, expression);
                } else {
                    auto arrayInit = parseArrayInit(allocator);
                    if(arrayInit) {
                        structValue->values[std::string(id->value)] = initializer(allocator, structValue, id, arrayInit);
                    } else {
                        error("expected an expression after ':' for struct member " + std::string(id->value));
                        return structValue;
                    }
                }
                readWhitespace();
                consumeToken(TokenType::CommaSym);
            } else {
                break;
            }
        } while(true);

        lexWhitespaceAndNewLines();

        auto rBrace = consumeOfType(TokenType::RBrace);

        if(!rBrace) {
            error("expected '}' for struct value");
            return structValue;
        }

        structValue->location = loc(start, rBrace->position);

        return structValue;

    }

    return nullptr;
}

bool Parser::lexStructValueTokens(unsigned back_start) {
    if(lexOperatorToken(TokenType::LBrace)) {

        unsigned start = tokens_size() - 1 - back_start;

        // lex struct member value tokens
        do {
            lexWhitespaceAndNewLines();
            auto id = consumeIdentifierOrKeyword();
            if(id) {
                storeIdentifier(id);
                lexWhitespaceToken();
                if(!lexOperatorToken(TokenType::ColonSym)) {
                    mal_value(start, "expected a ':' for initializing struct member " + std::string(id->value));
                    return true;
                }
                lexWhitespaceToken();
                if(!(lexExpressionTokens(true) || lexArrayInit())) {
                    mal_value(start, "expected an expression after ':' for struct member " + std::string(id->value));
                    return true;
                }
                lexWhitespaceToken();
                lexOperatorToken(TokenType::CommaSym);
            } else {
                break;
            }
        } while(true);

        lexWhitespaceAndNewLines();

        if(!lexOperatorToken(TokenType::RBrace)) {
            mal_value(start, "expected '}' for struct value");
            return true;
        }

        compound_from(start, LexTokenType::CompStructValue);

        return true;
    }
    return false;
}