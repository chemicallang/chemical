// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 24/02/2024.
//

#include "parser/Parser.h"
#include "ast/base/TypeBuilder.h"
#include "ast/values/CharValue.h"
#include "ast/values/StringValue.h"
#include "utils/StringHelpers.h"
#include "ast/values/NullValue.h"
#include "ast/values/BoolValue.h"
#include "ast/values/CharValue.h"
#include "ast/values/UCharValue.h"
#include "ast/values/ShortValue.h"
#include "ast/values/UShortValue.h"
#include "ast/values/IntValue.h"
#include "ast/values/UIntValue.h"
#include "ast/values/LongValue.h"
#include "ast/values/ULongValue.h"
#include "ast/values/NewTypedValue.h"
#include "ast/values/NewValue.h"
#include "ast/values/PlacementNewValue.h"
#include "ast/statements/PlacementNewNode.h"
#include "ast/values/BigIntValue.h"
#include "ast/values/IncDecValue.h"
#include "ast/values/UBigIntValue.h"
#include "ast/values/Int128Value.h"
#include "ast/values/UInt128Value.h"
#include "ast/values/FloatValue.h"
#include "ast/values/DoubleValue.h"
#include "ast/values/NumberValue.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/values/ArrayValue.h"
#include "ast/values/LambdaFunction.h"
#include "ast/values/CastedValue.h"
#include "ast/values/IsValue.h"
#include "ast/values/ValueNode.h"
#include "ast/structures/If.h"
#include "ast/statements/SwitchStatement.h"
#include "ast/structures/LoopBlock.h"
#include "parse_num.h"
#include "ast/statements/IncDecNode.h"
#include "ast/values/ComptimeValue.h"
#include "ast/values/UnsafeValue.h"
#include "ast/values/SizeOfValue.h"
#include "ast/values/AlignOfValue.h"
#include "ast/values/InValue.h"
#include "ast/types/LinkedType.h"
#include "ast/types/GenericType.h"
#include "ast/types/LinkedValueType.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/AccessChain.h"
#include "preprocess/2c/BufferedWriter.h"

bool read_escapable_char(const char** currentPtrPtr, const char* end, ScratchString<128>& str) {

    // load the current character
    const auto currentPtr = *currentPtrPtr;
    const char current = *currentPtr;

    // set next to current character
    *currentPtrPtr = currentPtr + 1;

    if(current != 'x') {
        const auto next = escaped_char(current);
        str.append_char(next);
        return next != current || next == '\\' || next == '\'' || next == '"';
    } else {

        // load next
        auto nextPtr = currentPtr + 1;
        if(nextPtr == end) return false;
        const auto next = *nextPtr;

        // load next next
        auto nextNextPtr = nextPtr + 1;
        if(nextNextPtr == end) return false;

        const auto nextNext = *nextNextPtr;
        if(next == '1' && nextNext == 'b') {
            // set current ptr next to \x1b| <-- here (at pipe)
            *currentPtrPtr = nextNextPtr + 1;
            str.append_char('\x1b');
            return true;
        } else {
            str.append_char(current);
            return false;
        }
    }
}

// returns whether the escape sequence is known or not
// if unknown reads it into the string without escaping it
chem::string_view escaped_view(ASTAllocator& allocator, BasicParser& parser, const chem::string_view& value) {
    ScratchString<128> str;

    // start and end
    auto currentPtr = value.data();
    auto end = value.data() + value.size();

    while(currentPtr != end) {
        const auto current = *currentPtr;
        if(current == '\\') {
            currentPtr++;
            if(currentPtr == end) {
                parser.error("couldn't escape the character");
                break;
            } else {
                if (!read_escapable_char(&currentPtr, end, str)) {
                    parser.error("couldn't escape the character");
                }
            }
        } else {
            str.append_char(current);
            currentPtr++;
        }
    }

    return parser.allocate_view(allocator, str.to_chem_view());

}

Value* Parser::parseCharValue(ASTAllocator& allocator) {
    auto& t = *token;
    if(t.type == TokenType::Char) {
        // consume the token
        token++;
        // the value will contain single quotes around it
        auto escaped = escaped_view(allocator, *this, t.value);
        switch(escaped.size()) {
            case 0:
                return new (allocator.allocate<CharValue>()) CharValue('\0', typeBuilder.getCharType(), loc_single(t));
            case 1:
                break;
            default:
                error("couldn't handle escape sequence in character value");
                break;
        }
        return new (allocator.allocate<CharValue>()) CharValue(*escaped.data(), typeBuilder.getCharType(), loc_single(t));
    } else {
        return nullptr;
    }
}

std::optional<chem::string_view> BasicParser::parseString(ASTAllocator& allocator) {
    auto& t = *token;
    switch(t.type) {
        case TokenType::String:
            token++;
            return escaped_view(allocator, *this, t.value);
        case TokenType::MultilineString:
            token++;
            return allocate_view(allocator, t.value);
        default:
            return std::nullopt;
    }
}

Value* Parser::parseStringValue(ASTAllocator& allocator) {
    auto& t = *token;
    switch(t.type) {
        case TokenType::String:
            token++;
            return new (allocator.allocate<StringValue>()) StringValue(escaped_view(allocator, *this, t.value), typeBuilder.getStringType(), loc_single(t));
        case TokenType::MultilineString:
            token++;
            return new (allocator.allocate<StringValue>()) StringValue(allocate_view(allocator, t.value), typeBuilder.getStringType(), loc_single(t));
        default:
            return nullptr;
    }
}

Value* Parser::parseNumberValue(ASTAllocator& allocator) {
    if(token->type == TokenType::Number) {
        // we take the mutable value of the token
        // every change we make to the token's value, we revert that change back
        // to keep the token value legitimate
        auto& value = token->value;
        const auto result = convert_number_to_value(allocator, typeBuilder, const_cast<char*>(value.data()), value.size(), is64Bit, loc_single(token));
        if(!result.error.empty()) {
            error(chem::string_view(result.error), token->position);
        }
        token++;
        return result.result;
    } else {
        return nullptr;
    }
}

VariableIdentifier* Parser::parseVariableIdentifier(ASTAllocator& allocator) {
    auto id = consumeIdentifierOrKeyword();
    if(id) {
        const auto varId = new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(allocate_view(allocator, id->value), loc_single(id));
#ifdef LSP_BUILD
        id->linked = varId;
#endif
        return varId;
    } else {
        return nullptr;
    }
}

Value* Parser::parseArrayInit(ASTAllocator& allocator) {
    auto token1 = consumeOfType(TokenType::LBracket);
    if(!token1){
        return nullptr;
    }
    auto arrayValue = new (allocator.allocate<ArrayValue>()) ArrayValue(nullptr, loc(token1, token), allocator);
    do {
        consumeNewLines();
        auto expr = parseExpressionOrArrayOrStruct(allocator);
        if(expr) {
            arrayValue->values.emplace_back(expr);
        } else {
            break;
        }
        consumeNewLines();
    } while (consumeToken(TokenType::CommaSym));
    if (!consumeToken(TokenType::RBracket)) {
        unexpected_error("expected a ']' for an array");
        return arrayValue;
    }
    arrayValue->getType()->set_array_size(arrayValue->array_size());
    return arrayValue;
}

inline NamedLinkedType* named_linked_type(Parser& parser, ASTAllocator& allocator, Token* id) {
    // type for the first identifier
    auto idType = new(allocator.allocate<NamedLinkedType>()) NamedLinkedType(parser.allocate_view(allocator, id->value));
#ifdef LSP_BUILD
    id->linked = idType;
#endif
    return idType;
}

/**
 * var x0 = new a <-- type
 * var x1 = new a<> <-- type
 * var x2 = new a<>() <-- value
 * var x3 = new a<>{} <-- struct value
 * var y = new a::b::c <-- type
 * var z = new a::b::c <int> <-- type
 * var a = new a::b::c <int> () <-- value
 * var b = new a::b::c () <-- value
 * var c = new a::b::c {} <-- value
 * var d = new a::b::c <int> {} <-- value
 */
void parseNewValueExpr(Parser& parser, ASTAllocator& allocator, Value*& outValue, TypeLoc& outTypeLoc) {

    auto id = parser.consumeIdentifier();
    if(id == nullptr) {
        // non identifier, probably new int
        outTypeLoc = parser.parseTypeLoc(allocator);
        return;
    }

    // allocated on stack, no allocation until push
    std::vector<ChainValue*> values;

    switch(parser.token->type) {

        // still a type, user wrote Thing[] or Thing*
        case TokenType::LBracket:
        case TokenType::MultiplySym:
        case TokenType::AmpersandSym:
            parser.token--;
            outTypeLoc = parser.parseTypeLoc(allocator);
            return;

        // user is writing a function call
        case TokenType::LParen:
            values.emplace_back(
                    new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(parser.allocate_view(allocator, id->value), parser.loc_single(id))
            );
            outValue = parser.parseFunctionCall(allocator, values);
            return;

            // user is writing a struct value
        case TokenType::LBrace:
            outValue = (Value*) parser.parseStructValue(allocator, named_linked_type(parser, allocator, id), id->position);
            return;

            // user is writing an access chain
        case TokenType::DoubleColonSym:
        case TokenType::DotSym: {

            const auto id_loc = parser.loc_single(id);

            // parse a single identifier
            values.emplace_back(
                    new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(parser.allocate_view(allocator, id->value), id_loc)
            );

            // parse a dot chain
            while(true) {

                switch(parser.token->type) {
                    case TokenType::DoubleColonSym: {
                        // set previous identifier is_ns
                        auto lastId = values.back();
                        auto last_id = lastId->as_identifier();
                        if (last_id) {
                            last_id->is_ns = true;
                        } else {
                            parser.error("double colon '::' after unknown value");
                        }
                        break;
                    }
                    case TokenType::DotSym:
                        break;
                    default:
                        goto end_loop;

                }

                parser.token++;

                auto next_id = parser.consumeIdentifier();
                if(next_id) {
                    values.emplace_back(
                            new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(parser.allocate_view(allocator, next_id->value), parser.loc_single(next_id))
                    );
                } else {
                    parser.error("expected an identifier after '.' or '::'");
                    break;
                }

            }
            end_loop:

            switch(parser.token->type) {
                case TokenType::LParen:
                    outValue = parser.parseFunctionCall(allocator, values);
                    return;
                case TokenType::LessThanSym: {

                    // we have a generic list
                    std::vector<TypeLoc> genArgs;
                    parser.parseGenericArgsList(genArgs, allocator);

                    switch (parser.token->type) {
                        case TokenType::LParen: {
                            const auto call = parser.parseFunctionCall(allocator, values);
                            call->generic_list = std::move(genArgs);
                            outValue = call;
                            return;
                        }
                        case TokenType::LBrace: {
                            const auto genType = new(allocator.allocate<GenericType>()) GenericType(parser.ref_type_from(allocator, values), std::move(genArgs));
                            outValue = (Value*) parser.parseStructValue(allocator, genType, id->position);
                            return;
                        }
                        default:
                            // This is a type
                            outTypeLoc = {
                                    new(allocator.allocate<GenericType>()) GenericType(parser.ref_type_from(allocator, values), std::move(genArgs)),
                                    parser.loc_single(id)
                            };
                            return;
                    }

                }
                default:
                    // This is a type
                    outTypeLoc = {
                            parser.ref_type_from(allocator, values),
                            parser.loc_single(id)
                    };
                    return;
            }

            // safe return
            return;
        }
            // a generic list is upcoming
        case TokenType::LessThanSym: {

            // we have a generic list
            std::vector<TypeLoc> genArgs;
            parser.parseGenericArgsList(genArgs, allocator);

            switch (parser.token->type) {
                case TokenType::LParen: {
                    // parse a single identifier
                    values.emplace_back(
                            new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(parser.allocate_view(allocator, id->value), parser.loc_single(id))
                    );
                    const auto call = parser.parseFunctionCall(allocator, values);
                    call->generic_list = std::move(genArgs);
                    outValue = call;
                    return;
                }
                case TokenType::LBrace: {
                    const auto genType = new(allocator.allocate<GenericType>()) GenericType(named_linked_type(parser, allocator, id), std::move(genArgs));
                    outValue = (Value*) parser.parseStructValue(allocator, genType, id->position);
                    return;
                }
                default:
                    outTypeLoc = {
                            new(allocator.allocate<GenericType>()) GenericType(named_linked_type(parser, allocator, id), std::move(genArgs)),
                            parser.loc_single(id)
                    };
                    return;
            }

            // safe return
            return;
        }
        default:
            outTypeLoc = {
                    named_linked_type(parser, allocator, id),
                    parser.loc_single(id)
            };
            return;
    }

}

/**
 * There are 4 types of new values
 * 1 : new int; (there's a type after the value) storage -> BaseType*
 * 2 : new thing { }; (allocate the struct on heap) storage -> Value*
 * 3 : new thing(); (allocate the struct on heap and call constructor on it) storage -> Value*
 * 4 : new (pointer) thing(); (just call the constructor with the given allocated pointer) storage -> Value*, Value*
 * 5 : new (pointer) thing { }; (just assign the values in the struct at the given pointer storage -> Value*, Value*
 * But we store them as three
 * a : NewTypedValue -> corresponds to case 1
 * b : NewValue -> corresponds to case 2 and 3
 * c : PlacementNewValue -> corresponds to case 4, 5
 */
Value* Parser::parseNewValue(ASTAllocator& allocator) {

    const auto new_tok = token;
    if(new_tok->type != TokenType::NewKw) {
        return nullptr;
    }
    token++;

    // placement new
    const auto t_type = token->type;
    if(t_type == TokenType::LParen) {
        auto new_value = new (allocator.allocate<PlacementNewValue>()) PlacementNewValue(nullptr, nullptr, loc_single(new_tok));
        token++;
        auto pointer_val = parseExpression(allocator);
        if(!pointer_val) {
            unexpected_error("expected a pointer value for placement new");
            return nullptr;
        }
        new_value->pointer = pointer_val;
        if(token->type != TokenType::RParen) {
            unexpected_error("expected a ')' after the pointer value in new expression");
        }
        token++;
        auto value = parseExpression(allocator, true);
        if(!value) {
            unexpected_error("expected a value for placement new expression");
            return nullptr;
        }
        new_value->value = value;
        return new_value;
    }

    Value* expr = nullptr;
    TypeLoc typeLoc(nullptr);

    parseNewValueExpr(*this, allocator, expr, typeLoc);

    if(expr == nullptr) {

        if(!typeLoc) {
            unexpected_error("expected type after new");
            return nullptr;
        }

        return new (allocator.allocate<NewTypedValue>()) NewTypedValue(typeLoc, loc_single(new_tok));

    } else {

        return new (allocator.allocate<NewValue>()) NewValue(expr, loc_single(new_tok));

    }

}

ASTNode* Parser::parsePlacementNewNode(ASTAllocator& allocator) {
    const auto new_tok = token;
    if(new_tok->type != TokenType::NewKw) {
        return nullptr;
    }
    token++;

    // placement new
    const auto t_type = token->type;
    if(t_type == TokenType::LParen) {
        auto new_value = new (allocator.allocate<PlacementNewNode>()) PlacementNewNode(nullptr, nullptr, loc_single(new_tok), parent_node);
        token++;
        auto pointer_val = parseExpression(allocator);
        if(!pointer_val) {
            unexpected_error("expected a pointer value for placement new");
            return nullptr;
        }
        new_value->value.pointer = pointer_val;
        if(token->type != TokenType::RParen) {
            unexpected_error("expected a ')' after the pointer value in new expression");
        }
        token++;
        auto value = parseExpression(allocator, true);
        if(!value) {
            unexpected_error("expected a value for placement new expression");
            return nullptr;
        }
        new_value->value.value = value;
        return new_value;
    }

    error("new value as a statement is not allowed, it can lead to memory leaks");
    return nullptr;

}

Value* Parser::parsePostIncDec(ASTAllocator& allocator, Value* value, Token* start_token) {
    switch(token->type) {
        case TokenType::DoublePlusSym: {
            token++;
            return new (allocator.allocate<IncDecValue>()) IncDecValue(value, true, true, loc_single(start_token));
        }
        case TokenType::DoubleMinusSym: {
            token++;
            return new (allocator.allocate<IncDecValue>()) IncDecValue(value, false, true, loc_single(start_token));
        }
        default:
            return value;
    }
}

Value* parseIsValue(Parser& parser, ASTAllocator& allocator, Value* value, Token* start_token, bool is_negating) {
    parser.token++;
    auto type = parser.parseTypeLoc(allocator);
    auto isValue = new(allocator.allocate<IsValue>()) IsValue(value, type, is_negating, parser.typeBuilder.getBoolType(), parser.loc_single(start_token));
    if (!type) {
        isValue->type = { (BaseType*) parser.typeBuilder.getVoidType(), ZERO_LOC };
        parser.unexpected_error("expected a type after 'is' in expression");
    }
    return isValue;
}

Value* parseInValue(Parser& parser, ASTAllocator& allocator, Value* value, Token* start_token, bool is_negating) {
    parser.token++;
    auto inValue = new(allocator.allocate<InValue>()) InValue(value, is_negating, parser.typeBuilder.getBoolType(), parser.loc_single(start_token));
    while(true) {
        const auto expr = parser.parseExpression(allocator, false, false);
        if(expr) {
            inValue->values.emplace_back(expr);
            if(!parser.consumeToken(TokenType::CommaSym)) {
                break;
            }
        } else {
            break;
        }
    }
    return inValue;
}

Value* Parser::parseAfterValue(ASTAllocator& allocator, Value* value, Token* start_token) {
    switch(token->type) {
        case TokenType::DoublePlusSym: {
            token++;
            return new (allocator.allocate<IncDecValue>()) IncDecValue(value, true, true, loc_single(start_token));
        }
        case TokenType::DoubleMinusSym: {
            token++;
            return new (allocator.allocate<IncDecValue>()) IncDecValue(value, false, true, loc_single(start_token));
        }
        case TokenType::AsKw: {
            token++;
            const auto loc = loc_single(start_token);
            auto type = parseTypeLoc(allocator);
            if (!type) {
                type = TypeLoc((BaseType*) typeBuilder.getVoidType(), loc);
                unexpected_error("expected a type for casting after 'as' in expression");
            }
            return new(allocator.allocate<CastedValue>()) CastedValue(value, type, loc);
        }
        case TokenType::NotSym: {
            token++;
            switch(token->type) {
                case TokenType::IsKw:
                    return parseIsValue(*this, allocator, value, start_token, true);
                case TokenType::InKw:
                    return parseInValue(*this, allocator, value, start_token, true);
                default:
                    error("expected 'is' after the '!'");
                    return value;
            }
        }
        case TokenType::IsKw:
            return parseIsValue(*this, allocator, value, start_token, false);
        case TokenType::InKw:
            return parseInValue(*this, allocator, value, start_token, false);
        default:
            return value;
    }
}

IncDecNode* Parser::parsePreIncDecNode(ASTAllocator& allocator, bool increment) {
    auto& t = *token;
    if(t.type == (increment ? TokenType::DoublePlusSym : TokenType::DoubleMinusSym)) {
        token++;
    } else {
        return nullptr;
    }
    const auto expr = parseAccessChainOrAddrOf(allocator);
    if(!expr) {
        if(increment) {
            unexpected_error("expected an expression after the pre increment");
        } else {
            unexpected_error("expected an expression after the pre decrement");
        }
    }
    return new (allocator.allocate<IncDecNode>()) IncDecNode(expr, increment, false, loc_single(t), parent_node);
}

Value* Parser::parsePreIncDecValue(ASTAllocator& allocator, bool increment) {
    auto& t = *token;
    if(t.type == (increment ? TokenType::DoublePlusSym : TokenType::DoubleMinusSym)) {
        token++;
    } else {
        return nullptr;
    }
    const auto expr = parseAccessChainOrAddrOf(allocator);
    if(!expr) {
        if(increment) {
            unexpected_error("expected an expression after the pre increment");
        } else {
            unexpected_error("expected an expression after the pre decrement");
        }
    }
    return new (allocator.allocate<IncDecValue>()) IncDecValue(expr, increment, false, loc_single(t));
}

Value* Parser::parseAccessChainOrValueNoAfter(ASTAllocator& allocator, bool parseStruct) {
    const auto start_token = token;
    switch(start_token->type) {
        case TokenType::IfKw:
            return (Value*) parseIfValue(allocator, false);
        case TokenType::SwitchKw:
            return (Value*) parseSwitchValue(allocator);
        case TokenType::LoopKw:
            return (Value*) parseLoopValue(allocator);
        case TokenType::DoublePlusSym:
            return (Value*) parsePreIncDecValue(allocator, true);
        case TokenType::DoubleMinusSym:
            return (Value*) parsePreIncDecValue(allocator, false);
        case TokenType::NewKw:
            return parseNewValue(allocator);
        case TokenType::Char:
            return (Value*) parseCharValue(allocator);
        case TokenType::String:
        case TokenType::MultilineString:
            return (Value*) parseStringValue(allocator);
        case TokenType::LogicalOrSym:
        case TokenType::PipeSym:
            return parseLambdaValue(allocator);
        case TokenType::Number:
            return (Value*) parseNumberValue(allocator);
        case TokenType::NotSym:
            return (Value*) parseNotValue(allocator);
        case TokenType::MinusSym:
            return (Value*) parseNegativeValue(allocator);
        case TokenType::HashMacro:
            return parseMacroValue(allocator);
        default:
            return parseAccessChainOrAddrOf(allocator, parseStruct);
    }
}

Value* Parser::parseAccessChainOrValue(ASTAllocator& allocator, bool parseStruct) {
    const auto start_token = token;
    switch(start_token->type) {
        case TokenType::IfKw:
            return (Value*) parseIfValue(allocator, false);
        case TokenType::SwitchKw:
            return (Value*) parseSwitchValue(allocator);
        case TokenType::LoopKw:
            return (Value*) parseLoopValue(allocator);
        case TokenType::DoublePlusSym:
            return parseAfterValue(allocator, (Value*) parsePreIncDecValue(allocator, true), start_token);
        case TokenType::DoubleMinusSym:
            return parseAfterValue(allocator, (Value*) parsePreIncDecValue(allocator, false), start_token);
        case TokenType::NewKw:
            return parseNewValue(allocator);
        case TokenType::Char:
            return parseAfterValue(allocator, (Value*) parseCharValue(allocator), start_token);
        case TokenType::String:
        case TokenType::MultilineString:
            return parseAfterValue(allocator, (Value*) parseStringValue(allocator), start_token);
        case TokenType::LogicalOrSym:
        case TokenType::PipeSym:
            return parseLambdaValue(allocator);
        case TokenType::Number:
            return parseAfterValue(allocator, (Value*) parseNumberValue(allocator), start_token);
        case TokenType::NotSym:
            return parseAfterValue(allocator, (Value*) parseNotValue(allocator), start_token);
        case TokenType::MinusSym:
            return parseAfterValue(allocator, (Value*) parseNegativeValue(allocator), start_token);
        case TokenType::HashMacro:
            return parseAfterValue(allocator, parseMacroValue(allocator), start_token);
        default:
            auto ac = parseAccessChainOrAddrOf(allocator, parseStruct);
            if(ac) {
                return parseAfterValue(allocator, ac, start_token);
            } else {
                return nullptr;
            }
    }
}


Value* Parser::parseSizeOfValue(ASTAllocator& allocator) {
    const auto tok = token;
    const auto first_type = tok->type;
    if(first_type == TokenType::LBrace || first_type == TokenType::LParen) {
        token++;
    } else {
        unexpected_error("expected '{' or '(' when parsing sizeof");
        return nullptr;
    }
    auto type = parseTypeLoc(allocator);
    if(type) {
        auto last = token;
        auto value = new (allocator.allocate<SizeOfValue>()) SizeOfValue(type, typeBuilder.getUBigIntType(), loc(tok, last));
        const auto last_type = last->type;
        if((first_type == TokenType::LBrace && last_type == TokenType::RBrace) || (first_type == TokenType::LParen && last_type == TokenType::RParen)) {
            token++;
        } else {
            unexpected_error("expected '}' or '}' after the type when parsing sizeof");
        }
        return value;
    } else {
        unexpected_error("expected a type in sizeof");
        return nullptr;
    }
}

Value* Parser::parseAlignOfValue(ASTAllocator& allocator) {
    auto tok = token;
    const auto first_type = tok->type;
    if(first_type == TokenType::LBrace || first_type == TokenType::LParen) {
        token++;
    } else {
        unexpected_error("expected '{' or '(' when parsing alignof");
        return nullptr;
    }
    auto type = parseTypeLoc(allocator);
    if(type) {
        auto last = token;
        auto value = new (allocator.allocate<AlignOfValue>()) AlignOfValue(type, typeBuilder.getUBigIntType(), loc(tok, last));
        const auto last_type = last->type;
        if((first_type == TokenType::LBrace && last_type == TokenType::RBrace) || (first_type == TokenType::LParen && last_type == TokenType::RParen)) {
            token++;
        } else {
            unexpected_error("expected '}' or '}' after the type when parsing sizeof");
        }
        return value;
    } else {
        error("expected a type in alignof");
        return nullptr;
    }
}

Value* Parser::parseUnsafeValue(ASTAllocator& allocator) {
    auto tok = token;
    const auto first_type = tok->type;
    if(first_type == TokenType::LBrace || first_type == TokenType::LParen) {
        token++;
    } else {
        unexpected_error("expected '{' or '(' when parsing comptime value");
        return nullptr;
    }
    auto expr = parseExpression(allocator);
    if(expr) {
        auto last = token;
        auto evaluated = new (allocator.allocate<UnsafeValue>()) UnsafeValue(&allocator, expr);
        const auto last_type = last->type;
        if((first_type == TokenType::LBrace && last_type == TokenType::RBrace) || (first_type == TokenType::LParen && last_type == TokenType::RParen)) {
            token++;
        } else {
            unexpected_error("expected '}' or '}' after the type when parsing comptime value");
        }
        return evaluated;
    } else {
        unexpected_error("expected a value in unsafe value");
        return nullptr;
    }
}

Value* Parser::parseComptimeValue(ASTAllocator& allocator) {
    auto tok = token;
    const auto first_type = tok->type;
    if(first_type == TokenType::LBrace || first_type == TokenType::LParen) {
        token++;
    } else {
        unexpected_error("expected '{' or '(' when parsing comptime value");
        return nullptr;
    }
    auto expr = parseExpression(allocator);
    if(expr) {
        auto last = token;
        auto evaluated = new (allocator.allocate<ComptimeValue>()) ComptimeValue(expr);
        const auto last_type = last->type;
        if((first_type == TokenType::LBrace && last_type == TokenType::RBrace) || (first_type == TokenType::LParen && last_type == TokenType::RParen)) {
            token++;
        } else {
            unexpected_error("expected '}' or '}' after the type when parsing comptime value");
        }
        return evaluated;
    } else {
        error("expected a value in #eval");
        return nullptr;
    }
}

ValueNode* Parser::parseValueNode(ASTAllocator& allocator) {
    auto acVal = parseExpression(allocator, true);
    if(acVal) {
        return new (allocator.allocate<ValueNode>()) ValueNode(acVal, parent_node, acVal->encoded_location());
    } else {
        return nullptr;
    }
}

const auto unk_bit_width_err = "unknown bit width given for a number";

Value* allocate_number_value(ASTAllocator& alloc, TypeBuilder& typeBuilder, unsigned long long value, SourceLocation location) {
    if(value > INT_MAX) {
        if(value > LONG_MAX) {
            return new(alloc.allocate<BigIntValue>()) BigIntValue((long long) value, typeBuilder.getBigIntType(), location);
        } else {
            return new(alloc.allocate<LongValue>()) LongValue((long) value, typeBuilder.getLongType(), location);
        }
    } else {
        return new (alloc.allocate<IntValue>()) IntValue((int) value, typeBuilder.getIntType(), location);
    }
}

parse_num_result<Value*> convert_number_to_value(ASTAllocator& alloc, TypeBuilder& typeBuilder, const char* value, std::size_t value_size, bool is64Bit, SourceLocation location) {

    // considers bitwidths
    // i8, i16, i32, i64, i128
    // ui8, ui16, ui32, ui64, ui128
    // u8, u16, u32, u64, u128

    const auto last_char_index = value_size - 1;
    const auto last_char = value[last_char_index];

    bool is_unsigned = false;
    // suffix index is the index where the suffix begins u8, i8
    std::size_t suffix_index = value_size;
    // bitwidth index is the index where bitwidth begins 8, 32, 64 <-- exact number
    std::size_t bit_width_index = value_size;
    // bit width size
    uint8_t bit_width_size = 0;

    if(value_size > 2) {
        const auto sec_last_index = value_size - 2;
        const auto sec_last = value[sec_last_index];
        // considering u8 or i8, or ui8
        if(sec_last == 'i') {
            if(value_size > 3 && value[value_size - 3] == 'u') {
                // u before i, means ui8 unsigned int
                is_unsigned = true;
                bit_width_index = last_char_index;
                suffix_index = value_size - 3;
            } else {
                // at second last position i8 or u8 only 8 is valid bitwidth as a single character
                bit_width_index = last_char_index;
                suffix_index = sec_last_index;
            }
            bit_width_size = 1;
        } else if(sec_last == 'u' && last_char != 'u' && last_char != 'U') {
            // at second last position i8 or u8 only 8 is valid as a single character
            is_unsigned = true;
            bit_width_index = last_char_index;
            suffix_index = sec_last_index;
            bit_width_size = 1;
        } else if(value_size > 3) {
            const auto third_last_index = value_size - 3;
            const auto third_last = value[third_last_index];
            // considering i16, i32, i64, u16, u32, u64, ui16, ui32, ui64
            if(third_last == 'i') {
                if(value_size > 4 && value[value_size - 4] == 'u') {
                    // u before i, means ui16, ui32 or ui64
                    is_unsigned = true;
                    bit_width_index = sec_last_index;
                    suffix_index = value_size - 4;
                } else {
                    bit_width_index = sec_last_index;
                    suffix_index = third_last_index;
                }
                bit_width_size = 2;
            } else if(third_last == 'u') {
                is_unsigned = true;
                bit_width_index = sec_last_index;
                suffix_index = third_last_index;
                bit_width_size = 2;
            } else if(value_size > 4) {
                // considering i128, u128, ui128
                const auto fourth_last_index = value_size - 4;
                const auto fourth_last = value[fourth_last_index];
                if(fourth_last == 'i') {
                    if(value_size > 5 && value[value_size - 5] == 'u') {
                        // u before i means ui128
                        is_unsigned = true;
                        bit_width_index = third_last_index;
                        suffix_index = value_size - 5;
                    } else {
                        suffix_index = fourth_last_index;
                        bit_width_index = third_last_index;
                    }
                    bit_width_size = 3;
                } else if(fourth_last == 'u') {
                    is_unsigned = true;
                    suffix_index = fourth_last_index;
                    bit_width_index = third_last_index;
                    bit_width_size = 3;
                }
            }
        }
    }

    std::string_view err;
    if(bit_width_size > 0) {
        const auto num = (int8_t) parse_num(value + bit_width_index, bit_width_size, strtol).result;
        // mut_value[suffix_index] = '\0';
        switch(num) {
            case 8:
                if(is_unsigned) {
                    const auto num_value = parse_num(value, suffix_index, strtoul);
                    return { new (alloc.allocate<UCharValue>()) UCharValue((char) num_value.result, typeBuilder.getUCharType(), location), num_value.error };
                } else {
                    const auto num_value = parse_num(value, suffix_index, strtol);
                    return { new (alloc.allocate<CharValue>()) CharValue((char) num_value.result, typeBuilder.getCharType(), location), num_value.error };
                }
            case 16:
                if(is_unsigned) {
                    const auto num_val = parse_num(value, suffix_index, strtoul);
                    return { new (alloc.allocate<UShortValue>()) UShortValue((unsigned short) num_val.result, typeBuilder.getUShortType(), location), num_val.error };
                } else {
                    const auto num_value = parse_num(value, suffix_index, strtol);
                    return { new (alloc.allocate<ShortValue>()) ShortValue((short) num_value.result, typeBuilder.getShortType(), location), num_value.error };
                }
                break;
            case 32:
                if(is_unsigned) {
                    const auto num_val = parse_num(value, suffix_index, strtoul);
                    return { new (alloc.allocate<UIntValue>()) UIntValue((unsigned int) num_val.result, typeBuilder.getUIntType(), location), num_val.error };
                } else {
                    const auto num_val = parse_num(value, suffix_index, strtol);
                    return { new (alloc.allocate<IntValue>()) IntValue((int) num_val.result, typeBuilder.getIntType(), location), num_val.error };
                }
                break;
            case 64:
                if(is_unsigned) {
                    const auto num_val = parse_num(value, suffix_index, strtoull);
                    return { new (alloc.allocate<UBigIntValue>()) UBigIntValue((unsigned long long) num_val.result, typeBuilder.getUBigIntType(), location), num_val.error };
                } else {
                    const auto num_val = parse_num(value, suffix_index, strtoll);
                    return { new (alloc.allocate<BigIntValue>()) BigIntValue((long long) num_val.result, typeBuilder.getBigIntType(), location), num_val.error };
                }
                break;
            case 128:
                if(is_unsigned) {
                    // TODO skipping as it requires converting two magnitudes
                    // TODO UInt128Value
                } else {
                    const auto is_negative = value[0] == '-';
                    const auto begin_index = is_negative ? 1 : 0;
                    const auto num_val = parse_num(value + begin_index, suffix_index, strtoull);
                    return { new (alloc.allocate<Int128Value>()) Int128Value(num_val.result, is_negative, typeBuilder.getInt128Type(), location), num_val.error };
                }
                break;
            default:
                err = unk_bit_width_err;
                break;
        }
    }

    // conversion
    switch(last_char) {
        case 'f':
        case 'F': {
            if(value[0] == '0' && (value[1] == 'x' || value[1] == 'X')) {
                // avoiding values like 0xFFFFFF
                goto final_block;
            } else {
                // mut_value[last_char_index] = '\0';
                const auto num_value = parse_num(value, last_char_index, strtof);
                // mut_value[last_char_index] = last_char;
                return {new(alloc.allocate<FloatValue>()) FloatValue(num_value.result, typeBuilder.getFloatType(), location), err.empty() ? num_value.error : err};
            }
        }
        case 'U':
        case 'u': {
            // mut_value[last_char_index] = '\0';
            const auto num_val = parse_num(value, suffix_index, strtoul);
            // mut_value[last_char_index] = last_char;
            return {new(alloc.allocate<UIntValue>()) UIntValue((unsigned int) num_val.result, typeBuilder.getUIntType(), location), num_val.error};
        }
        case 'i':
        case 'I': {
            // mut_value[last_char_index] = '\0';
            const auto num_val = parse_num(value, suffix_index, strtol);
            // mut_value[last_char_index] = last_char;
            return {new(alloc.allocate<IntValue>()) IntValue((int) num_val.result, typeBuilder.getIntType(), location), num_val.error};
        }
        case 'l':
        case 'L': {
            if(value_size > 2) {
                const auto sec_last_index = value_size - 2;
                const auto sec_last = value[sec_last_index];
                if(sec_last == 'u' || sec_last == 'U') {
                    // mut_value[sec_last_index] = '\0';
                    const auto num_value = parse_num(value, last_char_index, strtoul);
                    // mut_value[sec_last_index] = sec_last;
                    return { new (alloc.allocate<ULongValue>()) ULongValue(num_value.result, typeBuilder.getULongType(), location), err.empty() ? num_value.error : err };
                }
            }
            // mut_value[last_char_index] = '\0';
            const auto num_value = parse_num(value, last_char_index, strtol);
            // mut_value[last_char_index] = last_char;
            return { new (alloc.allocate<LongValue>()) LongValue(num_value.result, typeBuilder.getLongType(), location), err.empty() ? num_value.error : err };
        }
        default:
        final_block: {
            if (std::string_view(value, value_size).find('.') != std::string_view::npos) {
                // TODO we should judge by the length of the string to give better value (support float128 on large doubles)
                const auto num_value = parse_num(value, value_size, strtod);
                return { new(alloc.allocate<DoubleValue>()) DoubleValue(num_value.result, typeBuilder.getDoubleType(), location), err.empty() ? num_value.error : err };
            } else {
                const auto num_value = parse_num(value, value_size, strtoull);
                return { allocate_number_value(alloc, typeBuilder, num_value.result, location), err.empty() ? num_value.error : err };
            }
        }
    }
}