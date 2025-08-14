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

inline chem::string_view view_from(SourceProvider& provider, const char* data) {
    return { data, static_cast<std::size_t>(provider.current_data() - data) };
}

void read_digits(SourceProvider& provider) {
    while(true) {
        auto next = provider.peek();
        if(next != '\0' && std::isdigit(next)) {
            provider.increment();
        } else {
            return;
        }
    }
}

void read_alpha_or_digits(SourceProvider& provider) {
    while(true) {
        auto next = provider.peek();
        if(next != '\0' && std::isalnum(next)) {
            provider.increment();
        } else {
            return;
        }
    }
}

// assumes that a digit exists at current location
bool read_floating_digits(SourceProvider& provider) {
    read_digits(provider);
    if(provider.increment('.')) {
        read_digits(provider);
        return true;
    } else {
        return false;
    }
}

void read_number_suffix(SourceProvider& provider) {
    switch(provider.peek()) {
        case 'i':
            provider.increment();
            read_digits(provider);
            return;
        case 'u':
            provider.increment();
            if (provider.peek() == 'i') {
                provider.increment();
            } else if(provider.peek() == 'l') {
                provider.increment();
                return;
            }
            read_digits(provider);
            return;
        case 'U':
            provider.increment();
            if(provider.peek() == 'L') {
                provider.increment();
            }
            return;
        case 'l':
        case 'L':
        case 'f':
            provider.increment();
            return;
        default:
            return;
    }
}



// reads  numbers with suffixes 123i8, 124ui16 123u32 or 0x332b2
void read_number(SourceProvider& provider) {
    if(read_floating_digits(provider)) {
        auto next = provider.peek();
        if(next == 'f') {
            provider.increment();
        }
    } else {
        read_number_suffix(provider);
    }
}

// if number starts with zero, we call this function
// it allows us to check whether number is hex or bool like 0x34ffb3
// this also assumes that zero has already been consumed and put onto the string
void read_zero_starting_number(SourceProvider& provider) {
    switch(provider.peek()) {
        case 'X':
        case 'x':
        case 'o':
        case 'O':
            provider.increment();
            read_alpha_or_digits(provider);
            // TODO is this needed here ?
            read_number_suffix(provider);
            return;
        case 'b':
        case 'B':
            provider.increment();
            read_digits(provider);
            return;
    }
    return read_number(provider);
}

void read_current_line(SourceProvider& provider) {
    while(true) {
        auto p = provider.peek();
        if(p != '\0' && p != '\n' && p != '\r') {
            provider.increment();
        } else {
            return;
        }
    }
}

const char* read_multi_line_comment_text(SourceProvider& provider) {
    while(true) {
        const auto read = provider.readCharacter();
        if(read == '\0') {
            return provider.current_data();
        }
        if(read == '*' && provider.peek() == '/') {
            provider.increment();
            return provider.current_data() - 2;
        }
    }
}

const char* read_multi_line_string(SourceProvider& provider) {
    while(true) {
        switch(provider.readCharacter()) {
            case '"': {
                // check the second "
                const auto next = provider.peek();
                if (next == '"') {
                    // consume the second "
                    provider.increment();
                    // check the last "
                    if (provider.peek() == '"') {
                        // consume the last "
                        provider.increment();
                        // back three pointers (the three quotes)
                        return provider.current_data() - 3;
                    }
                }
                break;
            }
            case '\0':
                // no need to report an error if '\0', since next time unexpected token will be given
                return provider.current_data();
            default:
                break;
        }
    }
}

// returns whether the ending quote was found
bool read_quoted_string(SourceProvider& provider) {
    while(true) {
        switch(provider.readCharacter()) {
            case '\\':
                switch(provider.readCharacter()) {
                    case 'x':
                        provider.increment('1') && provider.increment('b');
                    default:
                        break;
                }
                break;
            case '"':
                return true;
            case '\n':
            case '\r':
                return false;
            default:
                break;
        }
    }
}

bool read_char_in_quotes(SourceProvider& provider) {
    switch(provider.readCharacter()) {
        case '\\':
            switch(provider.readCharacter()) {
                case 'x':
                    provider.increment('1') && provider.increment('b');
                default:
                    break;
            }
            return true;
        case '\'':
            return false;
        default:
            return true;
    }
}

void read_id(SourceProvider& provider) {
    while(true) {
        auto p = provider.peek();
        if(p != '\0' && (p == '_' || std::isalnum(p))) {
            provider.increment();
        } else {
            return;
        }
    }
}

void read_annotation_id(SourceProvider& provider) {
    while(true) {
        const auto p = provider.peek();
        switch(p) {
            case '\0':
            case '_':
            case '.':
            case ':':
                provider.increment();
                break;
            default:
                if(std::isalnum(p)) {
                    provider.increment();
                } else {
                    return;
                }
                break;
        }
    }
}

Token win_new_line(SourceProvider& provider, const Position& pos) {
    if(provider.peek() == '\n') {
        provider.increment();
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
    const auto curr_data_ptr = provider.current_data();
    const auto current = provider.readCharacter();
    switch(current) {
        case '\0':
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
                provider.increment();
                return Token(TokenType::DoublePlusSym, view_str(DoublePlusCStr), pos);
            } else {
                return Token(TokenType::PlusSym, view_str(PlusOpCStr), pos);
            }
        case '-':
            if(provider.peek() == '-') {
                provider.increment();
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
                provider.increment();
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
            read_annotation_id(provider);
            return Token(TokenType::Annotation, view_from(provider, curr_data_ptr), pos);
        }
        case '#': {
            read_annotation_id(provider);
            auto hashed_view = view_from(provider, curr_data_ptr);
            auto view = chem::string_view((hashed_view.data() + 1), hashed_view.size() - 1);
            auto found = binder->findHook(view, CBIFunctionType::InitializeLexer);
            if(found) {
                ((EmbeddedLexerInitializeFn) found)(this);
            }
            return Token(TokenType::HashMacro, hashed_view, pos);
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
                    provider.increment();
                    const auto data_ptr = provider.current_data();
                    read_current_line(provider);
                    return Token(TokenType::SingleLineComment, view_from(provider, data_ptr), pos);
                }
#endif
                read_current_line(provider);
                return getNextToken();
            } else if(p == '*') {
#ifdef LSP_BUILD
                if(keep_comments) {
                    provider.increment();
                    const auto start = provider.current_data();
                    const auto end = read_multi_line_comment_text(provider);
                    return Token(TokenType::MultiLineComment, chem::string_view(start, end - start), pos);
                }
#endif
                read_multi_line_comment_text(provider);
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
        case '\'': {
            const auto start = provider.current_data();
            if (read_char_in_quotes(provider)) {
                const auto end = provider.current_data();
                provider.increment('\'');
                return Token(TokenType::Char, chem::string_view(start, end - start), pos);
            } else {
                diagnoser.diagnostic("no value given inside single quotes", chem::string_view(file_path), pos, provider.position(), DiagSeverity::Error);
                return Token(TokenType::Char, view_str(EmptyCStr), pos);
            }
        }
        case '"':
            if(provider.peek() == '"') {
                provider.increment();
                if(provider.peek() == '"') {
                    provider.increment();
                    // here the multiline string begins
                    const auto start = provider.current_data();
                    const auto end = read_multi_line_string(provider);
                    return Token(TokenType::MultilineString, chem::string_view(start, end - start), pos);
                } else {
                    // empty string
                    return Token(TokenType::String, view_str(EmptyCStr), pos);
                }
            } else {
                // single line string
                const auto start = provider.current_data();
                const auto has_ending_quote = read_quoted_string(provider);
                if(!has_ending_quote) {
                    diagnoser.diagnostic("no ending quotes for the single line string", chem::string_view(file_path), pos, provider.position(), DiagSeverity::Error);
                }
                const auto end = provider.current_data() - (has_ending_quote ? 1 : 0);
                return Token(TokenType::String, chem::string_view(start, end - start), pos);
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
            read_zero_starting_number(provider);
            return Token(TokenType::Number, view_from(provider, curr_data_ptr), pos);
        default:
            break;
    }
    if(std::isdigit(current)) {
        read_number(provider);
        return Token(TokenType::Number, view_from(provider, curr_data_ptr), pos);
    } else if(current == '_' || std::isalpha(current)) {
        read_id(provider);
        auto view = view_from(provider, curr_data_ptr);
        auto found = keywords.find(view);
        if(found != keywords.end()) {
            return Token(found->second, found->first, pos);
        } else {
            return Token(TokenType::Identifier, view, pos);
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