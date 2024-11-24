// Copyright (c) Qinetik 2024.

#include "Lexer.h"
#include <unordered_map>
#include "cst/utils/StringHelpers.h"

const auto AnnotationAtCStr = "@";
const auto MacroHashCStr = "#";

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
const auto ColonOpCStr = ":";
const auto LogAndOpCStr = "&&";
const auto LogOrOpCStr = "||";
const auto CmpEqualOpCStr = "==";
const auto CmpGTOpCStr = ">";
const auto CmpLTOpCStr = "<";
const auto CmpGTEOpCStr = ">=";
const auto CmpLTEOpCStr = "<=";
const auto BitwiseAndOpCStr = "&";
const auto BitwiseOrOpCStr = "|";
const auto ScopeResOpCStr = "::";
const auto EqualOpCStr = "=";
const auto LambOpCStr = "=>";

const auto SingleQuotesOpCStr = "'";
const auto DoubleQuotesOpCStr = "\"";

const auto WSCStr = " ";
const auto NewlineCStr = "\n";
const auto NewlineRCStr = "\r";
const auto NewlineWinCStr = "\r\n";

const std::unordered_map<std::string_view, bool> keywords = {
        {

                // Local Level Statements
                { "for", true },
                { "switch", true },
                { "return", true },
                { "break", true },
                { "continue", true },
                { "destruct", true },
                { "provide", true },
                { "unreachable", true },
                { "init", true },
                { "try", true },
                { "catch", true },
                { "throw", true },
                { "if", true },
                { "else", true },
                { "while", true },
                { "do", true },

                // Values
                { "true", true },
                { "false", true },
                { "null", true },

                // Access Specifiers
                { "public", true },
                { "private", true },
                { "protected", true },
                { "internal", true },

                // Types

                // Signed Integer Types
                { "char", true },
                { "short", true },
                { "int", true },
                { "long", true },
                { "bigint", true },

                // Unsigned Integer Types
                { "uchar", true },
                { "ushort", true },
                { "uint", true },
                { "ulong", true },
                { "ubigint", true },

                // Other Types
                { "bool", true },
                { "any", true },
                { "double", true },
                { "longdouble", true },
                { "float", true },
                { "int128", true },
                { "uint128", true },
                { "float128", true },
                { "void", true },

                // Top Level Statements
                { "import", true },
                { "func", true },
                { "typealias", true },
                { "struct", true },
                { "union", true },
                { "variant", true },
                { "interface", true },
                { "impl", true },
                { "namespace", true },
                { "enum", true },
                { "var", true },
                { "using", true },
                { "comptime", true },

                // Other Keywords
                { "mut", true },
                { "self", true },
                { "this", true },
                { "as", true },
                { "is", true },
                { "dyn", true },
                { "const", true },
        }
};

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

// text that occurs inside chemical string, inside double quotes
// we stop at any backslash or double quote
void read_str_text(AllocatorStrBuilder& str, SourceProvider& provider) {
    while(true) {
        auto p = provider.peek();
        if(p != '\\' && p != '\"') {
            str.append(provider.readCharacter());
        } else {
            break;
        }
    }
}

// reads the character and escapes it, if 'n' is at current position, we can say backslash n escape seq
// if character is not a valid escape sequence unexpected token is returned
Token read_escape_seq(MultiStrAllocator& allocator, SourceProvider& provider, const Position& pos) {
    if(provider.peek() != -1) {
        return Token(TokenType::EscapeSeq, { allocator.two_chars_to_c_str('\\', provider.readCharacter()), 2 }, pos);
    } else {
        return Token(TokenType::Unexpected, { allocator.char_to_c_str('\\'), 1 }, pos);
    }
}

Token Lexer::getNextToken() {
    auto pos = provider.position();
    auto current = provider.readCharacter();
    if(current == -1) {
        return Token(TokenType::EndOfFile, { nullptr, 0 }, pos);
    }
    if(other_mode) {
        if(char_mode) {
            if(current == '\\') {
                return read_escape_seq(allocator, provider, pos);
            } else if(current == '\'') {
                char_mode = false;
                other_mode = false;
                auto pos2 = provider.position();
                provider.readCharacter();
                return Token(TokenType::Operator, { SingleQuotesOpCStr, 1 }, pos2);
            } else {
                return Token(TokenType::Char, { allocator.char_to_c_str(current), 1 }, pos);
            }
        } else if(string_mode) {
            if(current == '\\') {
                return read_escape_seq(allocator, provider, pos);
            } else if(current == '\"') {
                string_mode = false;
                other_mode = false;
                auto pos2 = provider.position();
                provider.readCharacter();
                return Token(TokenType::Operator, { SingleQuotesOpCStr, 1 }, pos2);
            }
            AllocatorStrBuilder str(current, allocator);
            read_str_text(str, provider);
            return Token(TokenType::String, str.finalize_view(), pos);
        } else {
#ifdef DEBUG
            throw std::runtime_error("unknown mode triggered");
#endif
        }
    }
    switch(current) {
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
        case '@':
            return Token(TokenType::Operator, { AnnotationAtCStr, 1 }, pos);
        case '#':
            return Token(TokenType::Operator, { MacroHashCStr, 1 }, pos);
        case '<':
            if(provider.increment('=')) {
                return Token(TokenType::Operator, { CmpLTEOpCStr, 2 }, pos);
            } else {
                return Token(TokenType::Operator, { CmpLTOpCStr, 1 }, pos);
            }
        case '>':
            if(provider.increment('=')) {
                return Token(TokenType::Operator, { CmpGTEOpCStr, 2 }, pos);
            } else {
                return Token(TokenType::Operator, { CmpGTOpCStr, 1 }, pos);
            }
        case '/': {
            auto p = provider.peek();
            if (p == '/') {
                AllocatorStrBuilder str(current, allocator);
                str.append(provider.readCharacter());
                read_current_line(str, provider);
                return Token(TokenType::SingleLineComment, str.finalize_view(), pos);
            } else if(p == '*') {
                AllocatorStrBuilder str(current, allocator);
                str.append(provider.readCharacter());
                read_multi_line_comment_text(str, provider);
                return Token(TokenType::MultiLineComment, str.finalize_view(), pos);
            } else {
                return Token(TokenType::Operator, {DivOpCStr, 1}, pos);
            }
        }
        case '&':
            if(provider.increment('&')) {
                return Token(TokenType::Operator, { LogAndOpCStr, 2 }, pos);
            } else {
                return Token(TokenType::Operator, { BitwiseAndOpCStr, 1 }, pos);
            }
        case '|':
            if(provider.increment('|')) {
                return Token(TokenType::Operator, { LogOrOpCStr, 2 }, pos);
            } else {
                return Token(TokenType::Operator, { BitwiseOrOpCStr, 1 }, pos);
            }
        case ':':
            if(provider.increment(':')) {
                return Token(TokenType::Operator, { ScopeResOpCStr, 2 }, pos);
            } else {
                return Token(TokenType::Operator, { ColonOpCStr, 1 }, pos);
            }
        case '=':
            if(provider.increment('=')) {
                return Token(TokenType::Operator, { CmpEqualOpCStr, 2 }, pos);
            } else if(provider.increment('>')) {
                return Token(TokenType::Operator, { LambOpCStr, 2 }, pos);
            } else {
                return Token(TokenType::Operator, { EqualOpCStr, 1 }, pos);
            }
        case '\'':
            other_mode = true;
            char_mode = true;
            return Token(TokenType::Operator, { SingleQuotesOpCStr, 1 }, pos);
        case '\"':
            other_mode = true;
            string_mode = true;
            return Token(TokenType::Operator, { DoubleQuotesOpCStr, 1 }, pos);
        case ' ':
            return Token(TokenType::Whitespace, { WSCStr, provider.readWhitespaces() + 1 }, pos);
        case '\t':
            return Token(TokenType::Whitespace, { WSCStr, provider.readWhitespaces() + 4 }, pos);
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
    if(std::isdigit(current)) {
        auto p = provider.peek();
        if(isWhitespace(p)) {
            return Token(TokenType::Number, { allocator.char_to_c_str(current), 1 }, pos);
        } else {
            AllocatorStrBuilder str(current, allocator);
            read_number(str, provider);
            return Token(TokenType::Number, str.finalize_view(), pos);
        }
    } else if(current == '_' || std::isalpha(current)) {
        AllocatorStrBuilder str(current, allocator);
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
    unit.tokens.reserve(250);
    while(true) {
        auto token = getNextToken();
        switch(token.type) {
            case TokenType::EndOfFile:
                goto exit_loop;
            case TokenType::Unexpected:
                unit.tokens.emplace_back(token);
                goto exit_loop;
            default:
                unit.tokens.emplace_back(token);
        }
    }
exit_loop:
    unit.allocator = std::move(allocator);
}