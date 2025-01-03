// Copyright (c) Qinetik 2024.

#include "parser/Parser.h"
#include "ast/values/StructValue.h"

StructMemberInitializer* initializer(ASTAllocator& allocator, StructValue* struct_value, chem::string_view id, Value* value) {
    return new (allocator.allocate<StructMemberInitializer>()) StructMemberInitializer(id, value, struct_value);
}

StructValue* Parser::parseStructValue(ASTAllocator& allocator, BaseType* refType, Position& start) {

    if(consumeToken(TokenType::LBrace)) {

        auto structValue = new (allocator.allocate<StructValue>()) StructValue(refType, nullptr, 0, parent_node);

        // lex struct member value tokens
        do {
            lexWhitespaceAndNewLines();
            auto id = consumeIdentifierOrKeyword();
            if(id) {
                readWhitespace();
                if(!consumeToken(TokenType::ColonSym)) {
                    error("expected a ':' for initializing struct member " + id->value.str());
                    return structValue;
                }
                readWhitespace();
                auto expression = parseExpression(allocator, true);
                if(expression) {
                    const auto id_view = allocate_view(allocator, id->value);
                    structValue->values[id_view] = initializer(allocator, structValue, id_view, expression);
                } else {
                    auto arrayInit = parseArrayInit(allocator);
                    if(arrayInit) {
                        const auto id_view = allocate_view(allocator, id->value);
                        structValue->values[id_view] = initializer(allocator, structValue, id_view, arrayInit);
                    } else {
                        error("expected an expression after ':' for struct member " + id->value.str());
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
