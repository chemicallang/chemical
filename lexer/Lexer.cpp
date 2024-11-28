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

const auto DoublePlusCStr = "++";
const auto DoubleMinusCStr = "--";

const auto PlusOpCStr = "+";
const auto MinusOpCStr = "-";
const auto MulOpCStr = "*";
const auto DivOpCStr = "/";
const auto ModOpCStr = "%";
const auto NotOpCStr = "!";
const auto DotOpCStr = ".";
const auto TripleDotCStr = "...";
const auto CommaOpCStr = ",";
const auto SemiColOpCStr = ";";
const auto ColonOpCStr = ":";
const auto LogAndOpCStr = "&&";
const auto LogOrOpCStr = "||";
const auto CmpNotEqualOpCStr = "!=";
const auto CmpEqualOpCStr = "==";
const auto CmpGTOpCStr = ">";
const auto CmpLTOpCStr = "<";
const auto CmpGTEOpCStr = ">=";
const auto CmpLTEOpCStr = "<=";
const auto BitwiseAndOpCStr = "&";
const auto BitwiseOrOpCStr = "|";
const auto BitwiseXOROpCStr = "^";
const auto LeftShiftOpCStr = "<<";
const auto RightShiftOpCStr = ">>";
const auto ScopeResOpCStr = "::";
const auto EqualOpCStr = "=";
const auto LambOpCStr = "=>";

const auto SingleQuotesOpCStr = "'";
const auto DoubleQuotesOpCStr = "\"";
const auto MultilineCommentEndCStr = "*/";

const auto WSCStr = " ";
const auto NewlineCStr = "\n";
const auto NewlineRCStr = "\r";
const auto NewlineWinCStr = "\r\n";

constexpr std::string_view view_str(const char* c_str) {
    return { c_str };
}

const std::unordered_map<std::string_view, TokenType> keywords = {
        {

                // Local Level Statements
                { "for", TokenType::ForKw },
                { "switch", TokenType::SwitchKw },
                { "loop", TokenType::LoopKw },
                { "return", TokenType::ReturnKw },
                { "break", TokenType::BreakKw },
                { "continue", TokenType::ContinueKw },
                { "destruct", TokenType::DestructKw },
                { "provide", TokenType::ProvideKw },
                { "default", TokenType::DefaultKw },
                { "unsafe", TokenType::UnsafeKw },
                { "unreachable", TokenType::UnreachableKw },
                { "init", TokenType::InitKw },
                { "try", TokenType::TryKw },
                { "catch", TokenType::CatchKw },
                { "throw", TokenType::ThrowKw },
                { "if", TokenType::IfKw },
                { "from", TokenType::FromKw },
                { "else", TokenType::ElseKw },
                { "while", TokenType::WhileKw },
                { "do", TokenType::DoKw },

                // Values
                { "true", TokenType::TrueKw },
                { "false", TokenType::FalseKw },
                { "null", TokenType::NullKw },

                // Access Specifiers
                { "public", TokenType::PublicKw },
                { "private", TokenType::PrivateKw },
                { "protected", TokenType::ProtectedKw },
                { "internal", TokenType::InternalKw },

                // Types

                // Signed Integer Types
                { "char", TokenType::CharKw },
                { "short", TokenType::ShortKw },
                { "int", TokenType::IntKw },
                { "long", TokenType::LongKw },
                { "bigint", TokenType::BigintKw },

                // Unsigned Integer Types
                { "uchar", TokenType::UCharKw },
                { "ushort", TokenType::UShortKw },
                { "uint", TokenType::UIntKw },
                { "ulong", TokenType::ULongKw },
                { "ubigint", TokenType::UBigintKw },

                // Other Types
                { "bool", TokenType::BoolKw },
                { "any", TokenType::AnyKw },
                { "double", TokenType::DoubleKw },
                { "longdouble", TokenType::LongdoubleKw },
                { "float", TokenType::FloatKw },
                { "int128", TokenType::Int128Kw },
                { "uint128", TokenType::Uint128Kw },
                { "float128", TokenType::Float128Kw },
                { "void", TokenType::VoidKw },

                // Top Level Statements
                { "import", TokenType::ImportKw },
                { "func", TokenType::FuncKw },
                { "typealias", TokenType::TypealiasKw },
                { "struct", TokenType::StructKw },
                { "union", TokenType::UnionKw },
                { "variant", TokenType::VariantKw },
                { "interface", TokenType::InterfaceKw },
                { "impl", TokenType::ImplKw },
                { "namespace", TokenType::NamespaceKw },
                { "enum", TokenType::EnumKw },
                { "var", TokenType::VarKw },
                { "using", TokenType::UsingKw },
                { "comptime", TokenType::ComptimeKw },

                // Other Keywords
                { "mut", TokenType::MutKw },
                { "self", TokenType::SelfKw },
                { "this", TokenType::ThisKw },
                { "as", TokenType::AsKw },
                { "is", TokenType::IsKw },
                { "dyn", TokenType::DynKw },
                { "const", TokenType::ConstKw },
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

// assumes that a digit exists at current location
bool read_floating_digits(AllocatorStrBuilder& str, SourceProvider& provider) {
    read_digits(str, provider);
    if(provider.increment('.')) {
        str.append('.');
        read_digits(str, provider);
        return true;
    } else {
        return false;
    }
}

// reads  numbers with suffixes 123i8, 124ui16 123u32
void read_number(AllocatorStrBuilder& str, SourceProvider& provider) {
    if(read_floating_digits(str, provider)) {
        auto next = provider.peek();
        if(next == 'f') {
            str.append(provider.readCharacter());
        }
    } else {
        switch(provider.peek()) {
            case 'i':
                str.append(provider.readCharacter());
                read_digits(str, provider);
                break;
            case 'u':
                str.append(provider.readCharacter());
                if (provider.peek() == 'i') {
                    str.append(provider.readCharacter());
                } else if(provider.peek() == 'l') {
                    str.append(provider.readCharacter());
                    return;
                }
                read_digits(str, provider);
                break;
            case 'U':
                if(provider.peek() == 'L') {
                    str.append(provider.readCharacter());
                }
                break;
            case 'l':
            case 'L':
                str.append(provider.readCharacter());
                break;
            default:
                break;
        }
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
        auto p = provider.peek();
        if(p == '\n' || p == '\r' || p == '*') {
            return;
        } else {
            str.append(provider.readCharacter());
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
        if(p != '\\' && p != '"' && p != '\n' && p != '\r') {
            str.append(provider.readCharacter());
        } else {
            break;
        }
    }
}

Token win_new_line(SourceProvider& provider, const Position& pos) {
    if(provider.peek() == '\n') {
        provider.readCharacter();
        return Token(TokenType::NewLine, { NewlineWinCStr, 2 }, pos);
    } else {
        return Token(TokenType::NewLine, { NewlineRCStr, 1 }, pos);
    }
}

// reads the character and escapes it, if 'n' is at current position, we can say backslash n escape seq
// if character is not a valid escape sequence unexpected token is returned
Token read_escape_seq_in_str(MultiStrAllocator& allocator, SourceProvider& provider, const Position& pos) {
    AllocatorStrBuilder builder('\\', allocator);
    while(true) {
        switch(provider.peek()) {
            case -1:
                return Token(TokenType::EndOfFile, { nullptr, 0 }, pos);
            case '"':
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                goto end_loop;
            default:
                auto c = provider.readCharacter();
                builder.append(c);
                if(c == 'x' && provider.increment('1')) {
                    builder.append('1');
                    if(provider.increment('b')) {
                        builder.append('b');
                    }
                }
                goto end_loop;
        }
    }
    end_loop:
    return Token(TokenType::EscapeSeq, builder.finalize_view(), pos);
}

Token read_escape_seq_in_char(MultiStrAllocator& allocator, SourceProvider& provider, const Position& pos) {
    AllocatorStrBuilder builder('\\', allocator);
    while(true) {
        switch(provider.peek()) {
            case -1:
                return Token(TokenType::EndOfFile, { nullptr, 0 }, pos);
            case '\'':
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                goto end_loop;
            default:
                builder.append(provider.readCharacter());
        }
    }
    end_loop:
    return Token(TokenType::EscapeSeq, builder.finalize_view(), pos);
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
                return read_escape_seq_in_char(allocator, provider, pos);
            } else if(current == '\'') {
                char_mode = false;
                other_mode = false;
                return Token(TokenType::SingleQuoteSym, view_str(SingleQuotesOpCStr), pos);
            } else {
                return Token(TokenType::Char, { allocator.char_to_c_str(current), 1 }, pos);
            }
        } else if(string_mode) {
            switch(current) {
                case '\\':
                    return read_escape_seq_in_str(allocator, provider, pos);
                case '"': {
                    string_mode = false;
                    other_mode = false;
                    return Token(TokenType::DoubleQuoteSym, view_str(DoubleQuotesOpCStr), pos);
                }
                case '\n':
                    return Token(TokenType::NewLine, view_str(NewlineCStr), pos);
                case '\r':
                    return win_new_line(provider, pos);
                default:
                    AllocatorStrBuilder str(current, allocator);
                    read_str_text(str, provider);
                    return Token(TokenType::String, str.finalize_view(), pos);
            }
        } else if(comment_mode) {
            switch(current) {
                case '*': {
                    if(provider.peek() == '/') {
                        comment_mode = false;
                        other_mode = false;
                        provider.readCharacter();
                        return Token(TokenType::MultiLineComment, {MultilineCommentEndCStr, 2 }, pos);
                    } else {
                        break;
                    }
                }
                case '\n':
                    return Token(TokenType::NewLine, { NewlineCStr, 1 }, pos);
                case '\r':
                    return win_new_line(provider, pos);
                default:
                    break;
            }
            AllocatorStrBuilder str(current, allocator);
            read_multi_line_comment_text(str, provider);
            return Token(TokenType::MultiLineComment, str.finalize_view(), pos);
        } else {
#ifdef DEBUG
            throw std::runtime_error("unknown mode triggered");
#endif
        }
    }
    switch(current) {
        case '{':
            return Token(TokenType::LBrace, view_str(LBraceCStr), pos);
        case '}':
            return Token(TokenType::RBrace, view_str(RBraceCStr), pos);
        case '(':
            return Token(TokenType::LParen, view_str(LParenCStr), pos);
        case ')':
            return Token(TokenType::RParen, view_str(RParenCStr), pos);
        case '[':
            return Token(TokenType::LBracket, view_str(LBracketCStr), pos);
        case ']':
            return Token(TokenType::RBracket, view_str(RBracketCStr), pos);
        case '+':
            if(provider.peek() == '+') {
                provider.readCharacter();
                return Token(TokenType::DoublePlusSym, view_str(DoublePlusCStr), pos);
            } else {
                return Token(TokenType::PlusSym, view_str(PlusOpCStr), pos);
            }
        case '-':
            if(provider.peek() == '-') {
                provider.readCharacter();
                return Token(TokenType::DoubleMinusSym, view_str(DoubleMinusCStr), pos);
            } else {
                return Token(TokenType::MinusSym, view_str(MinusOpCStr), pos);
            }
        case '*':
            return Token(TokenType::MultiplySym, view_str(MulOpCStr), pos);
        case '%':
            return Token(TokenType::ModSym, view_str(ModOpCStr), pos);
        case '^':
            return Token(TokenType::CaretUpSym, view_str(BitwiseXOROpCStr), pos);
        case '!':
            if(provider.increment('=')) {
                return Token(TokenType::NotEqualSym, view_str(CmpNotEqualOpCStr), pos);
            } else {
                return Token(TokenType::NotSym, view_str(NotOpCStr), pos);
            }
        case '.':
            if(provider.peek() == '.') {
                provider.readCharacter();
                if(provider.readCharacter() == '.') {
                    return Token(TokenType::TripleDotSym, view_str(TripleDotCStr), pos);
                } else {
                    return Token(TokenType::Unexpected, { nullptr, 0 }, pos);
                }
            } else {
                return Token(TokenType::DotSym, view_str(DotOpCStr), pos);
            }
        case ',':
            return Token(TokenType::CommaSym, view_str(CommaOpCStr), pos);
        case ';':
            return Token(TokenType::SemiColonSym, view_str(SemiColOpCStr), pos);
        case '@':
            return Token(TokenType::AtSym, view_str(AnnotationAtCStr), pos);
        case '#':
            return Token(TokenType::HashSym, view_str(MacroHashCStr), pos);
        case '<':
            if(provider.increment('=')) {
                return Token(TokenType::LessThanOrEqualSym, view_str(CmpLTEOpCStr), pos);
            } else if(provider.increment('<')) {
                return Token(TokenType::LeftShiftSym, view_str(LeftShiftOpCStr), pos);
            } else {
                return Token(TokenType::LessThanSym, view_str(CmpLTOpCStr), pos);
            }
        case '>':
            if(provider.increment('=')) {
                return Token(TokenType::GreaterThanOrEqualSym, view_str(CmpGTEOpCStr), pos);
            } else if(provider.increment('>')) {
                return Token(TokenType::RightShiftSym, view_str(RightShiftOpCStr), pos);
            } else {
                return Token(TokenType::GreaterThanSym, view_str(CmpGTOpCStr), pos);
            }
        case '/': {
            auto p = provider.peek();
            if (p == '/') {
                AllocatorStrBuilder str(current, allocator);
                str.append(provider.readCharacter());
                read_current_line(str, provider);
                return Token(TokenType::SingleLineComment, str.finalize_view(), pos);
            } else if(p == '*') {
                other_mode = true;
                comment_mode = true;
                AllocatorStrBuilder str(current, allocator);
                str.append(provider.readCharacter());
                read_multi_line_comment_text(str, provider);
                return Token(TokenType::MultiLineComment, str.finalize_view(), pos);
            } else {
                return Token(TokenType::DivideSym, view_str(DivOpCStr), pos);
            }
        }
        case '&':
            if(provider.increment('&')) {
                return Token(TokenType::LogicalAndSym, view_str(LogAndOpCStr), pos);
            } else {
                return Token(TokenType::AmpersandSym, view_str(BitwiseAndOpCStr), pos);
            }
        case '|':
            if(provider.increment('|')) {
                return Token(TokenType::LogicalOrSym, view_str(LogOrOpCStr), pos);
            } else {
                return Token(TokenType::PipeSym, view_str(BitwiseOrOpCStr), pos);
            }
        case ':':
            if(provider.increment(':')) {
                return Token(TokenType::DoubleColonSym, view_str(ScopeResOpCStr), pos);
            } else {
                return Token(TokenType::ColonSym, view_str(ColonOpCStr), pos);
            }
        case '=':
            if(provider.increment('=')) {
                return Token(TokenType::DoubleEqualSym, view_str(CmpEqualOpCStr), pos);
            } else if(provider.increment('>')) {
                return Token(TokenType::LambdaSym, view_str(LambOpCStr), pos);
            } else {
                return Token(TokenType::EqualSym, view_str(EqualOpCStr), pos);
            }
        case '\'':
            other_mode = true;
            char_mode = true;
            return Token(TokenType::SingleQuoteSym, view_str(SingleQuotesOpCStr), pos);
        case '"':
            other_mode = true;
            string_mode = true;
            return Token(TokenType::DoubleQuoteSym, view_str(DoubleQuotesOpCStr), pos);
        case ' ':
            return Token(TokenType::Whitespace, { WSCStr, provider.readWhitespaces() + 1 }, pos);
        case '\t':
            return Token(TokenType::Whitespace, { WSCStr, provider.readWhitespaces() + 4 }, pos);
        case '\n':
            return Token(TokenType::NewLine, view_str(NewlineCStr), pos);
        case '\r':
            return win_new_line(provider, pos);
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
            return Token(found->second, view, pos);
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
                // a single end of file token must be present so parser can stop
                unit.tokens.emplace_back(token);
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