// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 28/02/2024.
//

#include "parser/Parser.h"
#include "lexer/model/tokens/NumberToken.h"
#include "ast/values/CharValue.h"
#include "ast/values/StringValue.h"
#include "lexer/model/tokens/TypeToken.h"
#include "ast/values/DoubleValue.h"
#include "ast/values/FloatValue.h"

lex_ptr<ArrayValue> Parser::parseArrayValue() {

    // [1,2,3,4,5] // array of five elements, array type guessed from the type of the first element
    // [1,2,3,4](5) // array of five elements, with fifth element uninitialized, type guessed
    // [1,2,3,4]int(5) // array of five elements, with fifth element uninitialized
    // []int(5) // array of five elements, no initialization
    // []int(5,5) // multi dimensional, 5x5, no initialization

    // [1,2,3](2) error array size cannot be larger than the elements given
    // [](5) // error no element, and type, we can't guess type
    // []int // error int empty array, no size given, no elements, useless empty array

    // unsupported :
    // [[1,2],[1,2]] 2x2 array

    if(consume_op('[')) {
        // TODO support multi dimensional params
        std::vector<std::unique_ptr<Value>> params;
        do {
            auto param = parseExpression();
            if(param.has_value()) {
                params.emplace_back(std::move(param.value()));
            } else {
                break;
            }
        } while(consume_op(','));
        if(consume_op(']')) {
            lex_ptr<BaseType> type = parseType();
            if(!type.has_value() && params.empty()) {
                error("cannot guess array type, if there's no element in the array and type's not given");
                return std::nullopt;
            }
            std::vector<unsigned int> dimension_sizes;
            if(consume_op('(')) {
                do {
                    auto sizeVal = parseIntValue();
                    if (sizeVal.has_value()) {
                        dimension_sizes.push_back(sizeVal.value()->as_int());
                    } else {
                        break;
                    }
                } while (consume_op(','));
                if(!consume_op(')')) {
                    error("expected ')' after the array value declaration");
                    return std::nullopt;
                }
            }
            if(dimension_sizes.empty()) {
                if(params.empty()) {
                    error("array has no params & no size, creating an empty static array which is useless");
                    return std::nullopt;
                }
            } else {
                if(params.empty() || params.size() < dimension_sizes[0]) {
                    error("array elements cannot be larger than size of the array");
                    return std::nullopt;
                }
                for(const auto& size : dimension_sizes) {
                    if(size < 0) {
                        error("array size cannot be smaller than 0");
                        return std::nullopt;
                    }
                }
                // TODO check multi dimensional arrays sizes with their elements
            }
            return std::make_unique<ArrayValue>(std::move(params), std::move(type), std::move(dimension_sizes));
        } else {
            error("expected a ']' after the array declaration");
        }
    }
    return std::nullopt;
}

lex_ptr<Value> Parser::parseNumberValue() {
    if (tokens[position]->type() == LexTokenType::Number) {
        auto number = consume<NumberToken>();
        try {
            if (number->has_dot()) {
                if(number->is_float()) {
                    return std::make_unique<FloatValue>(std::stof(number->value));
                } else {
                    return std::make_unique<DoubleValue>(std::stod(number->value));
                }
            } else {
                return std::make_unique<IntValue>(std::stoi(number->value));
            }
        } catch (...) {
            error("invalid number given");
            return std::nullopt;
        }
    } else {
        return std::nullopt;
    }
}

lex_ptr<IntValue> Parser::parseIntValue() {
    if (tokens[position]->type() == LexTokenType::Number) {
        auto number = consume<NumberToken>();
        try {
            return std::make_unique<IntValue>(std::stoi(number->value));
        } catch (...) {
            error("invalid integer token");
            return std::nullopt;
        }
    } else {
        return std::nullopt;
    }
}

lex_ptr<BoolValue> Parser::parseBoolValue() {
    if(consume("true")) {
        return std::make_unique<BoolValue>(true);
    } else if(consume("false")){
        return std::make_unique<BoolValue>(false);
    }
    return std::nullopt;
}

lex_ptr<Value> Parser::parseValue() {
    auto strToken = consume_str_token();
    if(strToken.has_value()) {
        return std::make_unique<StringValue>(strToken.value());
    }
    auto charToken = consume_char_token();
    if(charToken.has_value()) {
        return std::make_unique<CharValue>(charToken.value());
    }
    auto boolVal = parseBoolValue();
    if(boolVal.has_value()) {
        return boolVal;
    }
    auto arrVal = parseArrayValue();
    if(arrVal.has_value()) {
        return arrVal;
    }
    return parseNumberValue();
}

lex_ptr<Value> Parser::parseAccessChainOrValue() {
    auto value = parseValue();
    if (value.has_value()) {
        return value;
    }
    auto chain = parseAccessChain();
    if (chain.has_value()) {
        if(chain.value()->values.size() == 1) {
            return std::move(chain.value()->values[0]);
        }
        return chain;
    }
    auto macro = parseMacroValue();
    if(macro.has_value()) {
        return macro;
    }
    return std::nullopt;
}