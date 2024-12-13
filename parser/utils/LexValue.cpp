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

Value* Parser::parseCharValue(ASTAllocator& allocator) {
    if(token->type == TokenType::SingleQuoteSym) {
        auto& start_pos = token->position;
        token++;
        auto val = consumeOfType(TokenType::Char);
        char char_value;
        if(val) {
            char_value = val->value[0];
        } else {
            auto seq = consumeOfType(TokenType::EscapeSeq);
            if(seq) {
                auto result = escapable_char(seq->value, 1);
                if(result.second == -1) {
                    error("unknown escape sequence", seq);
                    char_value = result.first;
                } else {
                    char_value = result.first;
                }
            } else {
                error("expected a character value or escape sequence after single quote");
                return nullptr;
            }
        }
        auto last = consumeOfType(TokenType::SingleQuoteSym);
        if(!last) {
            error("expected a ending quote when parsing a character value");
            return nullptr;
        }
        auto& end_pos = token->position;
        return new (allocator.allocate<CharValue>()) CharValue(char_value, loc(start_pos, end_pos));
    } else {
        return nullptr;
    }
}

Value* Parser::parseStringValue(ASTAllocator& allocator) {
    auto t = consumeOfType(TokenType::DoubleQuoteSym);
    if (t) {
        auto& start_pos = t->position;
        auto value = new (allocator.allocate<StringValue>()) StringValue("", loc_single(t));
        while(true) {
            const auto current = token;
            switch(current->type) {
                case TokenType::String:
                    value->value.append(current->value.view());
                    token++;
                    break;
                case TokenType::NewLine:
                    value->value.append(1, '\n');
                    token++;
                    break;
                case TokenType::Unexpected:
                case TokenType::EndOfFile:
                    goto loop_broken;
                case TokenType::EscapeSeq:{
                    auto result = escapable_char(current->value, 1);
                    if(result.second == -1) {
                        error("unknown escape sequence", current);
                        value->value.append(1, result.first);
                    } else {
                        value->value.append(1, result.first);
                    }
                    token++;
                    break;
                }
                case TokenType::DoubleQuoteSym:
                    goto loop_broken;
                default:
                    error("unexpected token", current);
                    goto loop_broken;
            }
        }
        loop_broken:
        auto end_quote = consumeOfType(TokenType::DoubleQuoteSym);
        if(!end_quote) {
            error("expected an ending quote for the string", token);
        }
        auto& last_pos = token->position;
        value->location = loc(start_pos, last_pos);
        return value;
    } else {
        return nullptr;
    }
}

Value* Parser::parseBoolValue(ASTAllocator& allocator) {
    auto first = consumeOfType(TokenType::TrueKw);
    if(first) {
        return new (allocator.allocate<BoolValue>()) BoolValue(true, loc_single(token));
    } else {
        auto second = consumeOfType(TokenType::FalseKw);
        if(second) {
            return new (allocator.allocate<BoolValue>()) BoolValue(false, loc_single(token));
        } else {
            return nullptr;
        }
    }
}

Value* Parser::parseNull(ASTAllocator& allocator) {
    auto current = consumeOfType(TokenType::NullKw);
    if(current) {
        return new (allocator.allocate<NullValue>()) NullValue(loc_single(token));
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
        return new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(id->value.str(), loc_single(id));
    } else {
        return nullptr;
    }
}

Value* Parser::parseAccessChainValueToken(ASTAllocator& allocator) {
    auto charVal = parseCharValue(allocator);
    if(charVal) return charVal;
    auto strVal = parseStringValue(allocator);
    if(strVal) return strVal;
    auto lambVal = parseLambdaValue(allocator);
    if(lambVal) return lambVal;
    auto numbVal = parseNumberValue(allocator);
    if(numbVal) return numbVal;
    return nullptr;
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
    auto ifStmt = parseIfStatement(allocator, true, true, false);
    if(ifStmt) {
        return ifStmt;
    }
    auto switchStmt = parseSwitchStatementBlock(allocator, true, true);
    if(switchStmt) {
        return switchStmt;
    }
    auto loopBlk = parseLoopBlockTokens(allocator, true);
    if(loopBlk) {
        return loopBlk;
    }
    auto acValue = parseAccessChainValueToken(allocator);
    if(acValue) {
        return parseAfterValue(allocator, acValue, start_token);
    }
    auto notValue = parseNotValue(allocator);
    if(notValue) {
        return parseAfterValue(allocator, (Value*) notValue, start_token);
    }
    auto negValue = parseNegativeValue(allocator);
    if(negValue) {
        return parseAfterValue(allocator, (Value*) negValue, start_token);
    }
    auto ac = parseAccessChainOrAddrOf(allocator, parseStruct);
    if(ac) {
        return parseAfterValue(allocator, ac, start_token);
    }
    auto macroVal = parseMacroValue(allocator);
    if(macroVal) {
        return parseAfterValue(allocator, macroVal, start_token);
    }
    return nullptr;
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