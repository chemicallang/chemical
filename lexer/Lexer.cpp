// Copyright (c) Qinetik 2024.

#include "Lexer.h"
#include <unordered_map>

Lexer::Lexer(
        std::string file_path,
        SourceProvider &provider,
        CompilerBinder* binder
) : file_path(std::move(file_path)), provider(provider), binder(binder), allocator(3000) {

}

bool isWhitespace(char p) {
    return p == ' ' || p == '\t' || p == '\n' || p == '\r';
}

// read digits into the string
void read_digits(AllocatorStrBuilder& str, SourceProvider& provider) {
    while(true) {
        auto next = provider.peek();
        if(std::isdigit(next)) {
            str.append(provider.readCharacter());
        } else {
            break;
        }
    }
}

// reads  numbers with suffixes 123i8, 124ui16 123u32
void read_number(AllocatorStrBuilder& str, SourceProvider& provider) {
    read_digits(str, provider);
    auto next = provider.peek();
    if(next == 'i') {
        str.append(provider.readCharacter());
        read_digits(str, provider);
    } else if(next == 'u') {
        str.append(next);
        if(provider.peek() == 'i') {
            str.append(provider.readCharacter());
        }
        read_digits(str, provider);
    }
}

void read_current_line(AllocatorStrBuilder& str, SourceProvider& provider) {
    while(true) {
        auto p = provider.peek();
        if(p != '\n' && p != '\r') {
            str.append(provider.readCharacter());
        } else {
            return;
        }
    }
}

void read_multi_line_comment_text(AllocatorStrBuilder& str, SourceProvider& provider) {
    while(true) {
        auto p = provider.readCharacter();
        if(p == '*' && provider.peek() == '/') {
            str.append('*');
            str.append(provider.readCharacter());
            return;
        } else {
            str.append(p);
        }
    }
}

void read_id(AllocatorStrBuilder& str, SourceProvider& provider) {
    while(true) {
        auto p = provider.peek();
        if(p == '_' || std::isalnum(p)) {
            str.append(provider.readCharacter());
        } else {
            return;
        }
    }
}

const std::unordered_map<std::string_view, bool> keywords = {
{
        { "import", true },
        { "if", true },
        { "while", true },
        { "do", true },
        { "true", true },
        { "false", true },
        { "func", true },
        { "null", true },
        { "public", true },
        { "private", true },
        { "bool", true },
        { "int", true },
        { "long", true },
        { "uint", true },
        { "ulong", true },
        { "mut", true },
        { "void", true },
        { "self", true },
        { "this", true },
        { "struct", true },
        { "interface", true },
        { "impl", true },
        { "for", true },
        { "as", true },
        { "is", true },
        { "var", true },
        { "dyn", true },
        { "switch", true },
        { "const", true },
        { "return", true }
    }
};

const auto LBraceCStr = "{";
const auto RBraceCStr = "}";
const auto LParenCStr = "(";
const auto RParenCStr = ")";
const auto LBracketCStr = "[";
const auto RBracketCStr = "]";

const auto PlusOpCStr = "+";
const auto MinusOpCStr = "-";
const auto MulOpCStr = "*";
const auto DivOpCStr = "/";
const auto ModOpCStr = "%";
const auto ExclOpCStr = "!";
const auto DotOpCStr = ".";
const auto CommaOpCStr = ",";
const auto SemiColOpCStr = ";";
const auto LogAndOpCStr = "&&";
const auto LogOrOpCStr = "||";
const auto BitwiseAndOpCStr = "&";
const auto BitwiseOrOpCStr = "|";

const auto NewlineCStr = "\n";
const auto NewlineRCStr = "\r";
const auto NewlineWinCStr = "\r\n";

Token Lexer::getNextToken() {
    auto pos = provider.position();
    auto c = provider.readCharacter();
    switch(c) {
        case '{':
            return Token(TokenType::LBrace, { LBraceCStr, 1 }, pos);
        case '}':
            return Token(TokenType::RBrace, { RBraceCStr, 1 }, pos);
        case '(':
            return Token(TokenType::LParen, { LParenCStr, 1 }, pos);
        case ')':
            return Token(TokenType::RParen, { RParenCStr, 1 }, pos);
        case '[':
            return Token(TokenType::LBracket, { LBracketCStr, 1 }, pos);
        case ']':
            return Token(TokenType::RBracket, { RBracketCStr, 1 }, pos);
        case '+':
            return Token(TokenType::Operator, { PlusOpCStr, 1 }, pos);
        case '-':
            return Token(TokenType::Operator, { MinusOpCStr, 1 }, pos);
        case '*':
            return Token(TokenType::Operator, { MulOpCStr, 1 }, pos);
        case '%':
            return Token(TokenType::Operator, { ModOpCStr, 1 }, pos);
        case '!':
            return Token(TokenType::Operator, { ExclOpCStr, 1 }, pos);
        case '.':
            return Token(TokenType::Operator, { DotOpCStr, 1 }, pos);
        case ',':
            return Token(TokenType::Operator, { CommaOpCStr, 1 }, pos);
        case ';':
            return Token(TokenType::Operator, { SemiColOpCStr, 1 }, pos);
        case -1:
            return Token(TokenType::EndOfFile, { nullptr, 0 }, pos);
        case '/': {
            auto p = provider.peek();
            if (p == '/') {
                AllocatorStrBuilder str(c, allocator);
                str.append(provider.readCharacter());
                read_current_line(str, provider);
                return Token(TokenType::SingleLineComment, str.finalize_view(), pos);
            } else if(p == '*') {
                AllocatorStrBuilder str(c, allocator);
                str.append(provider.readCharacter());
                read_multi_line_comment_text(str, provider);
                return Token(TokenType::MultiLineComment, str.finalize_view(), pos);
            } else {
                return Token(TokenType::Operator, {DivOpCStr, 1}, pos);
            }
        }
        case '&':
            if(provider.peek() == '&') {
                provider.readCharacter();
                return Token(TokenType::Operator, { LogAndOpCStr, 2 }, pos);
            } else {
                return Token(TokenType::Operator, { BitwiseAndOpCStr, 1 }, pos);
            }
        case '|':
            if(provider.peek() == '|') {
                provider.readCharacter();
                return Token(TokenType::Operator, { LogOrOpCStr, 2 }, pos);
            } else {
                return Token(TokenType::Operator, { BitwiseOrOpCStr, 1 }, pos);
            }
        case ' ':
            return Token(TokenType::Whitespace, { nullptr, provider.readWhitespaces() + 1 }, pos);
        case '\t':
            return Token(TokenType::Whitespace, { nullptr, provider.readWhitespaces() + 4 }, pos);
        case '\n':
            return Token(TokenType::NewLine, { NewlineCStr, 1 }, pos);
        case '\r':
            if(provider.peek() == '\n') {
                provider.readCharacter();
                return Token(TokenType::NewLine, { NewlineWinCStr, 2 }, pos);
            } else {
                return Token(TokenType::NewLine, { NewlineRCStr, 1 }, pos);
            }
        default:
            break;
    }
    if(std::isdigit(c)) {
        auto p = provider.peek();
        if(isWhitespace(p)) {
            return Token(TokenType::Number, { allocator.char_to_c_str(c), 1 }, pos);
        } else {
            AllocatorStrBuilder str(c, allocator);
            read_number(str, provider);
            return Token(TokenType::Number, str.finalize_view(), pos);
        }
    } else if(c == '_' || std::isalpha(c)) {
        AllocatorStrBuilder str(c, allocator);
        read_id(str, provider);
        auto view = str.finalize_view();
        auto found = keywords.find(view);
        if(found != keywords.end()) {
            return Token(TokenType::Keyword, view, pos);
        } else {
            return Token(TokenType::Identifier, view, pos);
        }
    }
    return Token(TokenType::Unexpected, { nullptr, 0 }, pos);
}

void to_string(chem::string& strOut, std::vector<Token>& tokens) {
    for(auto& t : tokens) {
        switch(t.type) {
            case TokenType::EndOfFile:
            case TokenType::Unexpected:
                continue;
            case TokenType::Whitespace: {
                unsigned i = 0;
                while(i < t.value.size()) {
                    strOut.append(' ');
                    i++;
                }
            }
            default:
                strOut.append(t.value);
        }
    }
}

void Lexer::getUnit(LexUnit& unit) {
    unit.allocator = std::move(allocator);
    unit.tokens.reserve(250);
    while(true) {
        auto token = getNextToken();
        switch(token.type){
            case TokenType::EndOfFile:
                return;
            case TokenType::Unexpected:
                // perform any resetting of lexer state here
                // reset();
                break;
            default:
                unit.tokens.emplace_back(token);
                break;
        }
    }
}