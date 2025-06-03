// Copyright (c) Chemical Language Foundation 2025.

#include "parser/Parser.h"
#include "ast/values/StructValue.h"

bool parseMem(Parser& parser, ASTAllocator& allocator, StructValue* value, const chem::string_view& id) {
    if(!parser.consumeToken(TokenType::ColonSym)) {
        parser.error() << "expected a ':' for initializing struct member " << id;
        return false;
    }
    auto expression = parser.parseExpressionOrArrayOrStruct(allocator);
    if(expression) {
        const auto id_view = parser.allocate_view(allocator, id);
        value->values.emplace(id_view, StructMemberInitializer { id_view, expression });
    } else {
        parser.error() << "expected an expression after ':' for struct member " << id;
        return false;
    }
    parser.consumeToken(TokenType::CommaSym);
    return true;
}

StructValue* Parser::parseStructValue(ASTAllocator& allocator, BaseType* refType, Position& start) {

    if(consumeToken(TokenType::LBrace)) {

        auto structValue = new (allocator.allocate<StructValue>()) StructValue({refType, loc_single(start, 1)}, 0);

        // lex struct member value tokens
        do {
            consumeNewLines();
            auto id = consumeIdentifierOrKeyword();
            if(id) {
#ifdef LSP_BUILD
                id->linked = structValue;
#endif
                if(!parseMem(*this, allocator, structValue, id->value)) {
                    return structValue;
                }
            } else {
                if(token->type == TokenType::String) {
                    auto& key = token->value;
                    token++;
                    if(!parseMem(*this, allocator, structValue, key)) {
                        break;
                    }
                } else {
                    break;
                }
            }
        } while(true);

        consumeNewLines();

        auto rBrace = consumeOfType(TokenType::RBrace);

        if(!rBrace) {
            error("expected '}' for struct value");
            return structValue;
        }

        structValue->set_encoded_location(loc(start, rBrace->position));

        return structValue;

    }

    return nullptr;
}