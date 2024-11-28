// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 24/02/2024.
//

#include "parser/Parser.h"
#include "ast/values/CharValue.h"
#include "ast/values/StringValue.h"
#include "cst/utils/StringHelpers.h"

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
        auto value = new (allocator.allocate<StringValue>()) StringValue("", 0);
        while(true) {
            const auto current = token;
            switch(current->type) {
                case TokenType::String:
                    value->value.append(current->value);
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

bool Parser::lexBoolToken() {
    auto first = consumeOfType(TokenType::TrueKw);
    if(first) {
        emplace(LexTokenType::Bool, first->position, "true");
        return true;
    } else {
        auto second = consumeOfType(TokenType::FalseKw);
        if(second) {
            emplace(LexTokenType::Bool, second->position, "true");
            return true;
        } else {
            return false;
        }
    }
}

bool Parser::lexNull() {
    auto current = consumeOfType(TokenType::NullKw);
    if(current) {
        emplace(LexTokenType::Null, current->position, "null");
        return true;
    } else {
        return false;
    }
}

bool Parser::lexValueToken() {
    return lexCharToken() || lexStringToken() || lexLambdaValue() || lexNumberToken() || lexBoolToken() ||
           lexNull();
}

bool Parser::lexAccessChainValueToken() {
    return lexCharToken() || lexStringToken() || lexLambdaValue() || lexNumberToken();
}

bool Parser::lexConstantValue() {
    return lexCharToken() || lexStringToken() || lexNumberToken() || lexBoolToken() ||
           lexNull() || lexAccessChainOrAddrOf() || lexAnnotationMacro();
}

bool Parser::lexArrayInit() {
    if (lexOperatorToken(TokenType::LBrace)) {
        auto start = tokens_size() - 1;
        do {
            lexWhitespaceAndNewLines();
            if (!(lexExpressionTokens(true) || lexArrayInit())) {
                break;
            }
            lexWhitespaceAndNewLines();
        } while (lexOperatorToken(TokenType::CommaSym));
        if (!lexOperatorToken(TokenType::RBrace)) {
            mal_value(start, "expected a '}' when lexing an array");
            return true;
        }
        lexWhitespaceToken();
        if (lexTypeTokens()) {
            lexWhitespaceToken();
            if (lexOperatorToken(TokenType::LParen)) {
                do {
                    lexWhitespaceToken();
                    if (!lexNumberToken()) {
                        break;
                    }
                    lexWhitespaceToken();
                } while (lexOperatorToken(TokenType::CommaSym));
                lexWhitespaceToken();
                if (!lexOperatorToken(TokenType::RParen)) {
                    mal_value(start, "expected a ')' when ending array size");
                    return true;
                }
            }
        }
        compound_from(start, LexTokenType::CompArrayValue);
        return true;
    } else {
        return false;
    }
}

bool Parser::lexAccessChainOrValue(bool lexStruct) {
    return lexIfBlockTokens(true, true, false) || lexSwitchStatementBlock(true, true) || lexLoopBlockTokens(true) || lexAccessChainValueToken() || lexAccessChainOrAddrOf(lexStruct) || lexAnnotationMacro();
}

bool Parser::lexValueNode() {
    if(lexAccessChainOrValue(true)) {
        auto start = tokens_size() - 1;
        compound_from(start, LexTokenType::CompValueNode);
        return true;
    } else {
        return false;
    }
}