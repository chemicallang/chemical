// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 24/02/2024.
//

#include "parser/Parser.h"
#include "ast/values/CharValue.h"
#include "ast/values/StringValue.h"
#include "cst/utils/StringHelpers.h"
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
#include "ast/values/BigIntValue.h"
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
#include "ast/statements/ValueWrapperNode.h"

Value* Parser::parseCharValue(ASTAllocator& allocator) {
    auto& t = *token;
    if(t.type == TokenType::Char) {
        // consume it
        token++;
        // the value will contain single quotes around it
        return new (allocator.allocate<CharValue>()) CharValue(*(t.value.data() + 1), loc_single(t));
    } else {
        return nullptr;
    }
}

Value* Parser::parseStringValue(ASTAllocator& allocator) {
    auto& t = *token;
    if(t.type == TokenType::String) {
        // consume it
        token++;
        // got the string
        const auto value = new (allocator.allocate<StringValue>()) StringValue("", loc_single(t));
        // the value will contain double quotes around it
        value->value.append(t.value.data() + 1, t.value.size() - 2);
        return value;
    } else {
        return nullptr;
    }
}

Value* Parser::parseNumberValue(ASTAllocator& allocator) {
    if(token->type == TokenType::Number) {
        // we take the mutable value of the token
        // every change we make to the token's value, we revert that change back
        // to keep the token value legitimate
        auto& value = token->value;
        const auto result = convert_number_to_value(allocator, const_cast<char*>(value.data()), value.size(), is64Bit, loc_single(token));
        if(!result.error.empty()) {
            error(result.error, token);
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
        return new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(allocate_view(allocator, id->value), loc_single(id));
    } else {
        return nullptr;
    }
}

Value* Parser::parseArrayInit(ASTAllocator& allocator) {
    auto token1 = consumeOfType(TokenType::LBrace);
    if (token1) {
        auto arrayValue = new (allocator.allocate<ArrayValue>()) ArrayValue({}, nullptr, { }, loc(token1, token), allocator);
        do {
            lexWhitespaceAndNewLines();
            auto expr = parseExpression(allocator, true);
            if(expr) {
                arrayValue->values.emplace_back(expr);
            } else {
                auto init = parseArrayInit(allocator);
                if(init) {
                    arrayValue->values.emplace_back(init);
                } else {
                    break;
                }
            }
            lexWhitespaceAndNewLines();
        } while (consumeToken(TokenType::CommaSym));
        if (!consumeToken(TokenType::RBrace)) {
            error("expected a '}' when lexing an array");
            return arrayValue;
        }
        lexWhitespaceToken();
        auto type = parseType(allocator);
        if(type) {
            lexWhitespaceToken();
            if (consumeToken(TokenType::LParen)) {
                do {
                    lexWhitespaceToken();
                    auto number = parseNumberValue(allocator);
                    if(number) {
                        arrayValue->sizes.emplace_back((unsigned int) number->get_the_int());
                    } else {
                        break;
                    }
                    lexWhitespaceToken();
                } while (consumeToken(TokenType::CommaSym));
                lexWhitespaceToken();
                if (!consumeToken(TokenType::RParen)) {
                    error("expected a ')' when ending array size");
                }
            }
        }
        arrayValue->created_type = new (allocator.allocate<ArrayType>()) ArrayType(type, arrayValue->array_size(), ZERO_LOC);
        return arrayValue;
    } else {
        return nullptr;
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
    readWhitespace();

    // placement new
    const auto t_type = token->type;
    if(t_type == TokenType::LParen) {
        auto new_value = new (allocator.allocate<PlacementNewValue>()) PlacementNewValue(nullptr, nullptr, loc_single(new_tok));
        token++;
        readWhitespace();
        auto pointer_val = parseExpression(allocator);
        new_value->pointer = pointer_val;
        readWhitespace();
        if(token->type != TokenType::RParen) {
            error("expected a ')' after the pointer value in new expression");
        }
        token++;
        readWhitespace();
        auto value = parseExpression(allocator);
        if(!value) {
            error("expected a value for placement new expression");
        }
        new_value->value = value;
        return new_value;
    }

    // int, long <-- all primitive types are keywords
    // *int, &int, dyn Thing, mut Thing
    if(t_type == TokenType::MultiplySym || t_type == TokenType::AmpersandSym || t_type == TokenType::DynKw || t_type == TokenType::MutKw || Token::isKeyword(t_type)) {

        auto type = parseType(allocator);
        if(!type) {
            error("expected type after new");
            return nullptr;
        }

        return new (allocator.allocate<NewTypedValue>()) NewTypedValue(type, loc_single(new_tok));

    } else {

        auto value = parseExpression(allocator);
        if(!value) {
            error("expected value after new");
            return nullptr;
        }

        return new (allocator.allocate<NewValue>()) NewValue(value, loc_single(new_tok));

    }

}

ASTNode* Parser::parseNewValueAsNode(ASTAllocator& allocator) {
    auto newValue = parseNewValue(allocator);
    if(newValue) {
        return new (allocator.allocate<ValueWrapperNode>()) ValueWrapperNode(newValue, parent_node);
    } else {
        return nullptr;
    }
}

Value* Parser::parseAfterValue(ASTAllocator& allocator, Value* value, Token* start_token) {
entry:
    switch(token->type) {
        case TokenType::AsKw: {
            token++;
            readWhitespace();
            auto type = parseType(allocator);
            auto casted_value = new(allocator.allocate<CastedValue>()) CastedValue(value, type, loc_single(start_token));
            if (!type) {
                error("expected a type for casting after 'as' in expression");
            }
            return casted_value;
        }
        case TokenType::IsKw: {
            token++;
            readWhitespace();
            auto type = parseType(allocator);
            auto isValue = new(allocator.allocate<IsValue>()) IsValue(value, type, false, loc_single(start_token));
            if (!type) {
                error("expected a type after 'is' or '!is' in expression");
            }
            return isValue;
        }
        case TokenType::Whitespace:
            token++;
            goto entry;
        default:
            return value;
    }
}

Value* Parser::parseAccessChainOrValue(ASTAllocator& allocator, bool parseStruct) {
    const auto start_token = token;
    switch(start_token->type) {
        case TokenType::IfKw:
            return parseIfStatement(allocator, true, true, false);
        case TokenType::SwitchKw:
            return parseSwitchStatementBlock(allocator, true, true);
        case TokenType::LoopKw:
            return parseLoopBlockTokens(allocator, true);
        case TokenType::NewKw:
            return parseNewValue(allocator);
        case TokenType::Char:
            return parseAfterValue(allocator, (Value*) parseCharValue(allocator), start_token);
        case TokenType::String:
            return parseAfterValue(allocator, (Value*) parseStringValue(allocator), start_token);
        case TokenType::LBracket:
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
            }
    }
}

ValueNode* Parser::parseValueNode(ASTAllocator& allocator) {
    auto acVal = parseAccessChainOrValue(allocator, true);
    if(acVal) {
        return new (allocator.allocate<ValueNode>()) ValueNode(acVal, parent_node, acVal->encoded_location());
    } else {
        return nullptr;
    }
}

const auto unk_bit_width_err = "unknown bit width given for a number";

parse_num_result<Value*> convert_number_to_value(ASTAllocator& alloc, char* mut_value, std::size_t value_size, bool is64Bit, SourceLocation location) {

    // considers bitwidths
    // i8, i16, i32, i64, i128
    // ui8, ui16, ui32, ui64, ui128
    // u8, u16, u32, u64, u128

    const char* value = mut_value;
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
        const auto suffix = mut_value[suffix_index];
        mut_value[suffix_index] = '\0';
        switch(num) {
            case 8:
                if(is_unsigned) {
                    const auto num_value = parse_num(value, suffix_index, strtoul);
                    return { new (alloc.allocate<UCharValue>()) UCharValue((char) num_value.result, location), num_value.error };
                } else {
                    const auto num_value = parse_num(value, suffix_index, strtol);
                    return { new (alloc.allocate<CharValue>()) CharValue((char) num_value.result, location), num_value.error };
                }
            case 16:
                if(is_unsigned) {
                    const auto num_val = parse_num(value, suffix_index, strtoul);
                    return { new (alloc.allocate<UShortValue>()) UShortValue((unsigned short) num_val.result, location), num_val.error };
                } else {
                    const auto num_value = parse_num(value, suffix_index, strtol);
                    return { new (alloc.allocate<ShortValue>()) ShortValue((short) num_value.result, location), num_value.error };
                }
                break;
            case 32:
                if(is_unsigned) {
                    const auto num_val = parse_num(value, suffix_index, strtoul);
                    return { new (alloc.allocate<UIntValue>()) UIntValue((unsigned int) num_val.result, location), num_val.error };
                } else {
                    const auto num_val = parse_num(value, suffix_index, strtol);
                    return { new (alloc.allocate<IntValue>()) IntValue((int) num_val.result, location), num_val.error };
                }
                break;
            case 64:
                if(is_unsigned) {
                    const auto num_val = parse_num(value, suffix_index, strtoull);
                    return { new (alloc.allocate<UBigIntValue>()) UBigIntValue((unsigned long long) num_val.result, location), num_val.error };
                } else {
                    const auto num_val = parse_num(value, suffix_index, strtoll);
                    return { new (alloc.allocate<BigIntValue>()) BigIntValue((long long) num_val.result, location), num_val.error };
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
                    return { new (alloc.allocate<Int128Value>()) Int128Value(num_val.result, is_negative, location), num_val.error };
                }
                break;
            default:
                err = unk_bit_width_err;
                break;
        }
        mut_value[suffix_index] = suffix;
    }

    // conversion
    switch(last_char) {
        case 'f':
        case 'F': {
            if(value[0] == '0' && (value[1] == 'x' || value[1] == 'X')) {
                // avoiding values like 0xFFFFFF
                goto final_block;
            } else {
                mut_value[last_char_index] = '\0';
                const auto num_value = parse_num(value, last_char_index, strtof);
                mut_value[last_char_index] = last_char;
                return {new(alloc.allocate<FloatValue>()) FloatValue(num_value.result, location), err.empty() ? num_value.error : err};
            }
        }
        case 'l':
        case 'L': {
            if(value_size > 2) {
                const auto sec_last_index = value_size - 2;
                const auto sec_last = value[sec_last_index];
                if(sec_last == 'u' || sec_last == 'U') {
                    mut_value[sec_last_index] = '\0';
                    const auto num_value = parse_num(value, last_char_index, strtoul);
                    mut_value[sec_last_index] = sec_last;
                    return { new (alloc.allocate<ULongValue>()) ULongValue(num_value.result, is64Bit, location), err.empty() ? num_value.error : err };
                }
            }
            mut_value[last_char_index] = '\0';
            const auto num_value = parse_num(value, last_char_index, strtol);
            mut_value[last_char_index] = last_char;
            return { new (alloc.allocate<LongValue>()) LongValue(num_value.result, is64Bit, location), err.empty() ? num_value.error : err };
        }
        default:
        final_block: {
            if (std::string_view(value, value_size).find('.') != std::string_view::npos) {
                // TODO we should judge by the length of the string to give better value (support float128 on large doubles)
                const auto num_value = parse_num(value, value_size, strtod);
                return { new(alloc.allocate<DoubleValue>()) DoubleValue(num_value.result, location), err.empty() ? num_value.error : err };
            } else {
                const auto num_value = parse_num(value, value_size, strtoll);
                return { new(alloc.allocate<NumberValue>()) NumberValue(num_value.result, location), err.empty() ? num_value.error : err };
            }
        }
    }
}