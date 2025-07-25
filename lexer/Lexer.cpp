// Copyright (c) Chemical Language Foundation 2025.

#include "Lexer.h"
#include <unordered_map>
#include "utils/StringHelpers.h"
#include "compiler/cbi/model/CompilerBinder.h"

const auto EmptyCStr = "";
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

const auto WSCStr = " ";
const auto NewlineCStr = "\n";
const auto NewlineRCStr = "\r";
const auto NewlineWinCStr = "\r\n";

constexpr chem::string_view view_str(const char* c_str) {
    return { c_str };
}

const std::unordered_map<chem::string_view, TokenType> keywords = {
        {

                // Local Level Statements
                { "for", TokenType::ForKw },
                { "switch", TokenType::SwitchKw },
                { "loop", TokenType::LoopKw },
                { "return", TokenType::ReturnKw },
                { "break", TokenType::BreakKw },
                { "new", TokenType::NewKw },
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
                { "sizeof", TokenType::SizeOfKw },
                { "alignof", TokenType::AlignOfKw },

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
                { "type", TokenType::TypeKw },
                { "struct", TokenType::StructKw },
                { "union", TokenType::UnionKw },
                { "variant", TokenType::VariantKw },
                { "interface", TokenType::InterfaceKw },
                { "impl", TokenType::ImplKw },
                { "namespace", TokenType::NamespaceKw },
                { "enum", TokenType::EnumKw },
                { "alias", TokenType::AliasKw },
                { "var", TokenType::VarKw },
                { "using", TokenType::UsingKw },
                { "comptime", TokenType::ComptimeKw },

                // Other Keywords
                { "mut", TokenType::MutKw },
                { "self", TokenType::SelfKw },
                { "Self", TokenType::SelfKw },
                { "this", TokenType::ThisKw },
                { "as", TokenType::AsKw },
                { "is", TokenType::IsKw },
                { "dyn", TokenType::DynKw },
                { "const", TokenType::ConstKw },
        }
};

// read digits into the string
void read_digits(SerialStrAllocator& str, SourceProvider& provider) {
    while(true) {
        auto next = provider.peek();
        if(next != -1 && std::isdigit(next)) {
            str.append(provider.readCharacter());
        } else {
            break;
        }
    }
}

void read_alpha_or_digits(SerialStrAllocator& str, SourceProvider& provider) {
    while(true) {
        auto next = provider.peek();
        if(next != -1 && std::isalnum(next)) {
            str.append(provider.readCharacter());
        } else {
            break;
        }
    }
}

// assumes that a digit exists at current location
bool read_floating_digits(SerialStrAllocator& str, SourceProvider& provider) {
    read_digits(str, provider);
    if(provider.increment('.')) {
        str.append('.');
        read_digits(str, provider);
        return true;
    } else {
        return false;
    }
}

void read_number_suffix(SerialStrAllocator& str, SourceProvider& provider) {
    switch(provider.peek()) {
        case 'i':
            str.append(provider.readCharacter());
            read_digits(str, provider);
            return;
        case 'u':
            str.append(provider.readCharacter());
            if (provider.peek() == 'i') {
                str.append(provider.readCharacter());
            } else if(provider.peek() == 'l') {
                str.append(provider.readCharacter());
                return;
            }
            read_digits(str, provider);
            return;
        case 'U':
            str.append(provider.readCharacter());
            if(provider.peek() == 'L') {
                str.append(provider.readCharacter());
            }
            return;
        case 'l':
        case 'L':
        case 'f':
            str.append(provider.readCharacter());
            return;
        default:
            return;
    }
}



// reads  numbers with suffixes 123i8, 124ui16 123u32 or 0x332b2
void read_number(SerialStrAllocator& str, SourceProvider& provider) {
    if(read_floating_digits(str, provider)) {
        auto next = provider.peek();
        if(next == 'f') {
            str.append(provider.readCharacter());
        }
    } else {
        read_number_suffix(str, provider);
    }
}

// if number starts with zero, we call this function
// it allows us to check whether number is hex or bool like 0x34ffb3
// this also assumes that zero has already been consumed and put onto the string
void read_zero_starting_number(SerialStrAllocator& str, SourceProvider& provider) {
    switch(provider.peek()) {
        case 'X':
        case 'x':
        case 'o':
        case 'O':
            str.append(provider.readCharacter());
            read_alpha_or_digits(str, provider);
            // TODO is this needed here ?
            read_number_suffix(str, provider);
            return;
        case 'b':
        case 'B':
            str.append(provider.readCharacter());
            read_digits(str, provider);
            return;
    }
    return read_number(str, provider);
}

void skip_current_line(SourceProvider& provider) {
    while(true) {
        auto p = provider.peek();
        if(p != -1 && p != '\n' && p != '\r') {
            provider.readCharacter();
        } else {
            return;
        }
    }
}

void read_current_line(SerialStrAllocator& str, SourceProvider& provider) {
    while(true) {
        auto p = provider.peek();
        if(p != -1 && p != '\n' && p != '\r') {
            str.append(provider.readCharacter());
        } else {
            return;
        }
    }
}

void skip_multi_line_comment_text(SourceProvider& provider) {
    while(true) {
        const auto read = provider.readCharacter();
        if(read == -1) {
            return;
        }
        if(read == '*' && provider.peek() == '/') {
            provider.readCharacter();
            return;
        }
    }
}

void read_multi_line_comment_text(SerialStrAllocator& str, SourceProvider& provider) {
    while(true) {
        const auto read = provider.readCharacter();
        if(read == -1) {
            return;
        }
        if(read != '*') {
            str.append(read);
        } else {
            if(provider.peek() == '/') {
                provider.readCharacter();
                return;
            } else {
                str.append(read);
            }
        }
    }
}

// reads an escape sequence without escaping it into the string
void read_escape_sequence(SerialStrAllocator& str, SourceProvider& provider) {
    char current = provider.readCharacter();
    if(current != 'x') {
        str.append(current);
    } else {
        if(provider.increment('1')) {
            if(provider.increment('b')) {
                str.append('x');
                str.append('1');
                str.append('b');
            } else {
                str.append('x');
                str.append('1');
            }
        } else {
            str.append('x');
        }
    }
}

// returns whether the escape sequence is known or not
// if unknown reads it into the string without escaping it
bool read_escapable_char(SerialStrAllocator &str, SourceProvider& provider) {
    char current = provider.readCharacter();
    if(current != 'x') {
        const auto next = escaped_char(current);
        str.append(next);
        return next != current || next == '\\' || next == '\'' || next == '"';
    } else {
        if(provider.increment('1') && provider.increment('b')) {
            str.append('\x1b');
            return true;
        } else {
            str.append(current);
            return false;
        }
    }
}

Token read_single_line_string(Lexer& lexer, SerialStrAllocator& str, SourceProvider& provider, const Position& pos) {
#ifdef LSP_BUILD
    bool escaping = false;
#endif
    while(true) {
        auto current = provider.readCharacter();
        switch(current) {
            case '\\':
#ifdef LSP_BUILD
                str.append(current);
                escaping = true;
#else
                if(!read_escapable_char(str, provider)) {
                    lexer.diagnoser.diagnostic("unknown escape sequence", chem::string_view(lexer.file_path), provider.position(), provider.position(), DiagSeverity::Error);
                }
#endif
                break;
            case '"':
#ifdef LSP_BUILD
                if(escaping) {
                    str.append('"');
                    escaping = false;
                    break;
                }
#endif
                // no need to report an error if -1, since next time unexpected token will be given
                return Token(TokenType::String, str.finalize_view(), pos);
            case -1:
                return Token(TokenType::String, str.finalize_view(), pos);
            case '\n':
            case '\r':
                lexer.diagnoser.diagnostic("single line string doesn't have a ending", chem::string_view(lexer.file_path), provider.position(), provider.position(), DiagSeverity::Error);
            default:
#ifdef LSP_BUILD
                escaping = false;
#endif
                str.append(current);
                break;
        }
    }
}

Token read_multi_line_string(Lexer& lexer, SerialStrAllocator& str, SourceProvider& provider, const Position& pos) {
    while(true) {
        auto current = provider.readCharacter();
        switch(current) {
            case '"': {
                // check the second "
                const auto next = provider.peek();
                if (next == '"') {
                    // consume the second "
                    provider.readCharacter();
                    // check the last "
                    if (provider.peek() == '"') {
                        // consume the last "
                        provider.readCharacter();
                        return Token(TokenType::MultilineString, str.finalize_view(), pos);
                    } else {
                        str.append('"');
                        str.append('"');
                    }
                } else {
                    str.append('"');
                }
                break;
            }
            case -1:
                // no need to report an error if -1, since next time unexpected token will be given
                return Token(TokenType::MultilineString, str.finalize_view(), pos);
            case '\n':
                str.append('\n');
                break;
            case '\r':
                if(provider.peek() == '\n') {
                    // only append a \n for \r\n
                    str.append(provider.readCharacter());
                } else {
                    // don't know what a single \r is doing here, outputting it as it is
                    str.append('\r');
                }
                break;
            default:
                str.append(current);
                break;
        }
    }
}

Token read_character_token(Lexer& lexer, SerialStrAllocator& str, SourceProvider& provider, const Position& pos) {
    auto current = provider.readCharacter();
    switch(current) {
        case '\\':
#ifdef LSP_BUILD
            str.append(current);
            read_escape_sequence(str, provider);
#else
            if(!read_escapable_char(str, provider)) {
                lexer.diagnoser.diagnostic("unknown escape sequence", chem::string_view(lexer.file_path), provider.position(), provider.position(), DiagSeverity::Error);
            }
#endif
            break;
        case '\'':
            lexer.diagnoser.diagnostic("no value given inside single quotes", chem::string_view(lexer.file_path), pos, provider.position(), DiagSeverity::Error);
            return Token(TokenType::Char, str.finalize_view(), pos);
        case -1:
            return Token(TokenType::Char, str.finalize_view(), pos);
        default:
            str.append(current);
            break;
    }
    if(provider.peek() == '\'') {
        provider.readCharacter();
        return Token(TokenType::Char, str.finalize_view(), pos);
    } else {
        lexer.diagnoser.diagnostic("expected a single quote after the value", chem::string_view(lexer.file_path), provider.position(), provider.position(), DiagSeverity::Error);
        read_current_line(str, provider);
        return Token(TokenType::Char, str.finalize_view(), pos);
    }
}

void read_id(SerialStrAllocator& str, SourceProvider& provider) {
    while(true) {
        auto p = provider.peek();
        if(p != -1 && (p == '_' || std::isalnum(p))) {
            str.append(provider.readCharacter());
        } else {
            return;
        }
    }
}

void read_annotation_id(SerialStrAllocator& str, SourceProvider& provider) {
    while(true) {
        const auto p = provider.peek();
        switch(p) {
            case -1:
            case '_':
            case '.':
            case ':':
                str.append(provider.readCharacter());
                break;
            default:
                if(std::isalnum(p)) {
                    str.append(provider.readCharacter());
                } else {
                    return;
                }
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

Token Lexer::getNextToken() {
    auto pos = provider.position();
    if(other_mode) {
        if(user_mode) {
            Token t;
            user_lexer.subroutine(&t, user_lexer.instance, this);
            return t;
        } else {
#ifdef DEBUG
            throw std::runtime_error("unknown mode triggered");
#endif
        }
    }
    const auto current = provider.readCharacter();
    switch(current) {
        case -1:
            return Token(TokenType::EndOfFile, { "", 0 }, pos);
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
                    diagnoser.diagnostic("unexpected dot symbol '.'", chem::string_view(file_path), provider.position(), provider.position(), DiagSeverity::Error);
                    return Token(TokenType::Unexpected, { "", 0 }, pos);
                }
            } else {
                return Token(TokenType::DotSym, view_str(DotOpCStr), pos);
            }
        case ',':
            return Token(TokenType::CommaSym, view_str(CommaOpCStr), pos);
        case ';':
            return Token(TokenType::SemiColonSym, view_str(SemiColOpCStr), pos);
        case '@': {
            str.append('@');
            read_annotation_id(str, provider);
            return Token(TokenType::Annotation, str.finalize_view(), pos);
        }
        case '#': {
            str.append('#');
            read_annotation_id(str, provider);
            auto view = chem::string_view((str.data + 1), str.length - 1);
            auto found = binder->findHook(view, CBIFunctionType::InitializeLexer);
            if(found) {
                ((EmbeddedLexerInitializeFn) found)(this);
            }
            return Token(TokenType::HashMacro, str.finalize_view(), pos);
        }
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
#ifdef LSP_BUILD
                if(keep_comments) {
                    provider.readCharacter();
                    read_current_line(str, provider);
                    return Token(TokenType::SingleLineComment, str.finalize_view(), pos);
                }
#endif
                skip_current_line(provider);
                return getNextToken();
            } else if(p == '*') {
#ifdef LSP_BUILD
                if(keep_comments) {
                    provider.readCharacter();
                    read_multi_line_comment_text(str, provider);
                    return Token(TokenType::MultiLineComment, str.finalize_view(), pos);
                }
#endif
                skip_multi_line_comment_text(provider);
                return getNextToken();
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
            return read_character_token(*this, str, provider, pos);
        case '"':
            if(provider.peek() == '"') {
                provider.readCharacter();
                if(provider.peek() == '"') {
                    provider.readCharacter();
                    // here the multiline string begins
                    return read_multi_line_string(*this, str, provider, pos);
                } else {
                    // empty string
                    return Token(TokenType::String, view_str(EmptyCStr), pos);
                }
            } else {
                // single line string
                return read_single_line_string(*this, str, provider, pos);
            }
        case ' ':
        case '\t':
#ifdef LSP_BUILD
            if(lex_whitespace) {
                return Token(TokenType::Whitespace, { WSCStr, provider.readWhitespaces() + (current == '\t' ? 4 : 1) }, pos);
            }
#endif
            // skip the whitespaces
            provider.skipWhitespaces();
            return getNextToken();
        case '\n':
            return Token(TokenType::NewLine, view_str(NewlineCStr), pos);
        case '\r':
            return win_new_line(provider, pos);
        case '0':
            str.append('0');
            read_zero_starting_number(str, provider);
            return Token(TokenType::Number, str.finalize_view(), pos);
        default:
            break;
    }
    if(std::isdigit(current)) {
        str.append(current);
        read_number(str, provider);
        return Token(TokenType::Number, str.finalize_view(), pos);
    } else if(current == '_' || std::isalpha(current)) {
        str.append(current);
        read_id(str, provider);
        auto view = str.current_view();
        auto found = keywords.find(view);
        if(found != keywords.end()) {
            str.deallocate();
            return Token(found->second, found->first, pos);
        } else {
            return Token(TokenType::Identifier, str.finalize_view(), pos);
        }
    }
    diagnoser.diagnostic("unexpected token", chem::string_view(file_path), provider.position(), provider.position(), DiagSeverity::Error);
    return Token(TokenType::Unexpected, { "", 0 }, pos);
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

void Lexer::getTokens(std::vector<Token>& tokens) {
    tokens.reserve(250);
    while(true) {
        auto token = getNextToken();
        switch(token.type) {
            case TokenType::EndOfFile:
                // a single end of file token must be present so parser can stop
                tokens.emplace_back(token);
                return;
            case TokenType::Unexpected:
                tokens.emplace_back(token);
                return;
            default:
                tokens.emplace_back(token);
        }
    }
}

void Lexer::getUnit(LexUnit& unit) {
    getTokens(unit.tokens);
exit_loop:
    unit.allocator = std::move(str);
}