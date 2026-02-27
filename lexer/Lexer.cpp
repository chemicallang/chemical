// Copyright (c) Chemical Language Foundation 2025.

#include "Lexer.h"
#include <unordered_map>
#include "utils/StringHelpers.h"
#include "compiler/cbi/model/CompilerBinder.h"
#include "std/except.h"

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
const auto DollarCStr = "$";
const auto QuestionMarkCStr = "?";
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

const auto AnnotMacroStartErr = "annotation or macro must start with a valid character that is not a digit";

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
                { "dealloc", TokenType::DeallocKw },
                { "delete", TokenType::DeleteKw },
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

                // chemical signed integer types
                { "i8", TokenType::I8Kw },
                { "i16", TokenType::I16Kw },
                { "i32", TokenType::I32Kw },
                { "i64", TokenType::I64Kw },

                // C like Signed Integer Types
                { "char", TokenType::CharKw },
                { "short", TokenType::ShortKw },
                { "int", TokenType::IntKw },
                { "long", TokenType::LongKw },
                { "longlong", TokenType::LongLongKw },
                // TODO: bigint type deprecated
                { "bigint", TokenType::BigIntKw },

                // chemical unsigned integer types
                { "u8", TokenType::U8Kw },
                { "u16", TokenType::U16Kw },
                { "u32", TokenType::U32Kw },
                { "u64", TokenType::U64Kw },

                // C like Unsigned Integer Types
                { "uchar", TokenType::UCharKw },
                { "ushort", TokenType::UShortKw },
                { "uint", TokenType::UIntKw },
                { "ulong", TokenType::ULongKw },
                { "ulonglong", TokenType::ULongLongKw },
                // TODO: ubigint type deprecated
                { "ubigint", TokenType::UBigIntKw },

                // Other Types
                { "bool", TokenType::BoolKw },
                { "any", TokenType::AnyKw },
                { "double", TokenType::DoubleKw },
                { "longdouble", TokenType::LongdoubleKw },
                { "float", TokenType::FloatKw },
                { "int128", TokenType::Int128Kw },
                { "uint128", TokenType::UInt128Kw },
                { "float128", TokenType::Float128Kw },
                { "void", TokenType::VoidKw },

                // Top Level Statements
                { "import", TokenType::ImportKw },
                { "export", TokenType::ExportKw },
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
                { "in", TokenType::InKw },
                { "dyn", TokenType::DynKw },
                { "zeroed", TokenType::ZeroedKw },
                { "const", TokenType::ConstKw },
        }
};

inline chem::string_view view_from(SourceProvider& provider, const char* data) {
    return { data, static_cast<std::size_t>(provider.current_data() - data) };
}

// ---------- ascii helpers ---------------
static inline bool is_ascii_digit(char c) {
    return c >= '0' && c <= '9';
}
static inline bool is_ascii_letter_or_digit(char c) {
    return (c >= 'a' && c <= 'z') || is_ascii_digit(c) || (c >= 'A' && c <= 'Z');
}

// ---------- helpers (fast inline checks) ----------
static inline bool is_ascii_digit(uint32_t cp) {
    return cp >= '0' && cp <= '9';
}
static inline bool is_ascii_letter_or_digit(uint32_t cp) {
    return  (cp >= 'a' && cp <= 'z') || is_ascii_digit(cp) || (cp >= 'A' && cp <= 'Z');
}
static inline bool is_ascii_letter_or_underscore(uint32_t cp) noexcept {
    return (cp == '_') || (cp >= 'A' && cp <= 'Z') || (cp >= 'a' && cp <= 'z');
}
static inline bool is_ascii_letter_digit_or_underscore(uint32_t cp) noexcept {
    return is_ascii_letter_or_underscore(cp) || is_ascii_digit(cp);
}

// ---------- noncharacter, private-use, surrogate checks ----------
static inline bool is_noncharacter(uint32_t cp) noexcept {
    // Range U+FDD0..U+FDEF
    if (cp >= 0xFDD0 && cp <= 0xFDEF) return true;
    // Values whose low 16 bits are 0xFFFE or 0xFFFF are noncharacters:
    // U+FFFE, U+FFFF, U+1FFFE, U+1FFFF, ..., U+10FFFE, U+10FFFF
    if ((cp & 0xFFFE) == 0xFFFE && cp >= 0xFFFE && cp <= 0x10FFFF) return true;
    return false;
}

static inline bool is_private_use(uint32_t cp) noexcept {
    // BMP private use area and planes 15/16
    if (cp >= 0xE000 && cp <= 0xF8FF) return true;
    if (cp >= 0xF0000 && cp <= 0xFFFFD) return true;
    if (cp >= 0x100000 && cp <= 0x10FFFD) return true;
    return false;
}

static inline bool is_surrogate(uint32_t cp) noexcept {
    return (cp >= 0xD800 && cp <= 0xDFFF);
}

// ---------- blacklists: controls, separators, punctuation, symbol-like ----------
static inline bool is_control_or_format(uint32_t cp) noexcept {
    // C0/C1 control characters
    if (cp <= 0x1F) return true;
    if (cp >= 0x7F && cp <= 0x9F) return true;
    // Some format chars that are dangerous / invisible (we disallow)
    // (Exceptions: we allow ZERO WIDTH NON-JOINER (U+200C) and ZERO WIDTH JOINER (U+200D) below)
    if ((cp >= 0x200E && cp <= 0x200F) || // LRM/RLM
        (cp >= 0x202A && cp <= 0x202E) || // bidi overrides
        (cp >= 0x2060 && cp <= 0x206F)    // invisible format / control-like
            ) return true;
    return false;
}

static inline bool is_space_separator(uint32_t cp) noexcept {
    // various space separators (Unicode Zs)
    if (cp == 0x00A0) return true;
    if (cp == 0x1680) return true;
    if (cp >= 0x2000 && cp <= 0x200A) return true;
    if (cp == 0x2028 || cp == 0x2029) return true; // line/paragraph separator
    if (cp == 0x202F) return true;
    if (cp == 0x205F) return true;
    if (cp == 0x3000) return true; // ideographic space
    return false;
}

static inline bool is_punctuation_or_symbol_block(uint32_t cp) noexcept {
    // block-level blacklists for punctuation/symbols (non-exhaustive but broad)
    // NOTE: we intentionally do NOT blacklist the Letterlike Symbols block (2100..214F)
    // because some languages expect characters there to be allowed. Adjust if you want.
    if (cp >= 0x2000 && cp <= 0x206F) return true; // General Punctuation
    if (cp >= 0x2E00 && cp <= 0x2E7F) return true; // Supplemental Punctuation
    if (cp >= 0x3000 && cp <= 0x303F) return true; // CJK Symbols and Punctuation
    if (cp >= 0x2190 && cp <= 0x21FF) return true; // Arrows
    if (cp >= 0x2200 && cp <= 0x22FF) return true; // Mathematical Operators
    if (cp >= 0x2300 && cp <= 0x23FF) return true; // Misc Technical
    if (cp >= 0x2460 && cp <= 0x24FF) return true; // Enclosed Alphanumerics
    if (cp >= 0x2500 && cp <= 0x257F) return true; // Box Drawing
    if (cp >= 0x25A0 && cp <= 0x25FF) return true; // Geometric Shapes
    if (cp >= 0x2600 && cp <= 0x26FF) return true; // Misc Symbols
    if (cp >= 0x2700 && cp <= 0x27BF) return true; // Dingbats
    if (cp >= 0x2B00 && cp <= 0x2BFF) return true; // Misc Symbols & Arrows
    if (cp >= 0x1F000 && cp <= 0x1F9FF) return true; // many emoji/pictographs (covers several emoji ranges)
    if (cp >= 0x1FA70 && cp <= 0x1FAFF) return true; // Symbols & Pictographs Extended-A
    if (cp >= 0x2300 && cp <= 0x23FF) return true; // Misc Technical (repeat ok)
    if (cp >= 0xFE00 && cp <= 0xFE0F) return false; // variation selectors: treat specially (allow combination)
    // Currency symbols
    if (cp >= 0x20A0 && cp <= 0x20CF) return true;
    return false;
}

// ---------- digits detection (common script digit ranges) ----------
static inline bool is_unicode_digit_common(uint32_t cp) noexcept {
    // ASCII digits handled via ASCII fast path; here handle common non-ASCII digit blocks.
    // This covers the vast majority of scripts' decimal digits; extend with generator for full coverage.
    if (cp >= 0x0660 && cp <= 0x0669) return true; // Arabic-Indic
    if (cp >= 0x06F0 && cp <= 0x06F9) return true; // Extended Arabic-Indic
    if (cp >= 0x07C0 && cp <= 0x07C9) return true; // NKo digits
    if (cp >= 0x0966 && cp <= 0x096F) return true; // Devanagari
    if (cp >= 0x09E6 && cp <= 0x09EF) return true; // Bengali
    if (cp >= 0x0A66 && cp <= 0x0A6F) return true; // Gurmukhi
    if (cp >= 0x0AE6 && cp <= 0x0AEF) return true; // Gujarati
    if (cp >= 0x0B66 && cp <= 0x0B6F) return true; // Oriya
    if (cp >= 0x0BE6 && cp <= 0x0BEF) return true; // Tamil
    if (cp >= 0x0C66 && cp <= 0x0C6F) return true; // Telugu
    if (cp >= 0x0CE6 && cp <= 0x0CEF) return true; // Kannada
    if (cp >= 0x0D66 && cp <= 0x0D6F) return true; // Malayalam
    if (cp >= 0x0E50 && cp <= 0x0E59) return true; // Thai
    if (cp >= 0x0ED0 && cp <= 0x0ED9) return true; // Lao
    if (cp >= 0x0F20 && cp <= 0x0F29) return true; // Tibetan
    if (cp >= 0x1040 && cp <= 0x1049) return true; // Myanmar
    if (cp >= 0x17E0 && cp <= 0x17E9) return true; // Khmer
    if (cp >= 0x1810 && cp <= 0x1819) return true; // Mongolian
    if (cp >= 0x1946 && cp <= 0x194F) return true; // Limbu
    if (cp >= 0x19D0 && cp <= 0x19D9) return true; // New Tai Lue
    if (cp >= 0x1A80 && cp <= 0x1A89) return true; // Tai Tham
    if (cp >= 0x1B50 && cp <= 0x1B59) return true; // Balinese
    if (cp >= 0x1BB0 && cp <= 0x1BB9) return true; // Sundanese
    if (cp >= 0xFF10 && cp <= 0xFF19) return true; // Fullwidth digits
    // There are more numeric ranges; add via offline generator if you want absolute coverage.
    return false;
}

// ---------- Zero-width exceptions allowed in continuation ----------
static inline bool is_zero_width_joiner_or_non_joiner(uint32_t cp) noexcept {
    return (cp == 0x200C || cp == 0x200D);
}

// ---------- Top-level predicates (blacklist strategy) ----------
static inline bool isIdentifierStart(uint32_t cp) noexcept {
    // 1) ASCII fast-path: very common and must be super-fast
    if (cp < 0x80) {
        return is_ascii_letter_or_underscore(cp);
    }

    // invalid scalar checks
    if (cp > 0x10FFFF) return false;
    if (is_surrogate(cp)) return false;
    if (is_noncharacter(cp)) return false;
    if (is_private_use(cp)) return false;
    if (is_control_or_format(cp)) return false;
    if (is_space_separator(cp)) return false;

    // 2) Disallow symbol/punctuation heavy blocks (emoji, math, arrows, dingbats, etc.)
    if (is_punctuation_or_symbol_block(cp)) return false;

    // 3) Reject digits as start (covers many scripts)
    // ASCII digits were handled in the ASCII fast-path.
    if (is_unicode_digit_common(cp)) return false;

    // If we reached here, it's not in the disallowed blacklists above -> allow.
    // This accepts letters from pretty much any script, combining marks (rare at start),
    // many letterlike symbols that some languages expect, etc.
    return true;
}

static inline bool isIdentifierContinue(uint32_t cp) noexcept {
    // 1) ASCII fast-path
    if (cp < 0x80) {
        return is_ascii_letter_digit_or_underscore(cp);
    }

    // invalid scalar checks
    if (cp > 0x10FFFF) return false;
    if (is_surrogate(cp)) return false;
    if (is_noncharacter(cp)) return false;
    if (is_private_use(cp)) return false;
    if (is_control_or_format(cp)) return false;
    if (is_space_separator(cp)) return false;

    // 2) Reject symbol/punctuation heavy blocks
    if (is_punctuation_or_symbol_block(cp)) return false;

    // 3) Allow zero-width joiner / non-joiner (useful in some scripts)
    if (is_zero_width_joiner_or_non_joiner(cp)) return true;

    // 4) Combining marks and other diacritics are allowed (they are not blacklisted above),
    //    and many digit ranges are allowed in continuation (we accept them by default).
    //    The only digits explicitly denied are in the start check — continuation accepts digits.

    // If we reached here, accept.
    return true;
}

void read_digits(SourceProvider& provider) {
    while(true) {
        auto next = provider.peek();
        if(next != '\0' && is_ascii_digit(next)) {
            provider.increment();
        } else {
            return;
        }
    }
}

void read_alpha_or_digits(SourceProvider& provider) {
    while(true) {
        auto next = provider.peek();
        if(next != '\0' && is_ascii_letter_or_digit(next)) {
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

// true -> ending quote found
// false -> string expression found
// nullopt -> both weren't found, error out
static inline void consume_hex_digits(SourceProvider& provider) {
    auto p = provider.peek();
    if ((p >= '0' && p <= '9') || (p >= 'a' && p <= 'f') || (p >= 'A' && p <= 'F')) {
        provider.increment();
        p = provider.peek();
        if ((p >= '0' && p <= '9') || (p >= 'a' && p <= 'f') || (p >= 'A' && p <= 'F')) {
            provider.increment();
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

// true -> ending quote found
// false -> string expression found
// nullopt -> both weren't found, error out
std::optional<bool> read_quoted_string2(SourceProvider& provider) {
    while(true) {
        switch(provider.peek()) {
            case '\\':
                provider.increment();
                switch(provider.readCharacter()) {
                    case 'x':
                        consume_hex_digits(provider);
                        break;
                    case 'u':
                        if (provider.increment('{')) {
                            while (!provider.eof() && provider.peek() != '}') {
                                provider.increment();
                            }
                            provider.increment('}');
                        } else {
                            for (int i = 0; i < 4 && !provider.eof(); i++) {
                                provider.increment();
                            }
                        }
                        break;
                    default:
                        break;
                }
                continue;
            case '"':
                provider.increment();
                return true;
            case '{':
                // does not consume the '{'
                return false;
            case '\n':
            case '\r':
                provider.increment();
                return std::nullopt;
            default:
                provider.increment();
                continue;
        }
    }
}

// true -> ending quote found
// false -> string expression found
// nullopt -> both weren't found, error out
std::optional<bool> read_backtick_string(SourceProvider& provider) {
    while(true) {
        switch(provider.peek()) {
            case '`':
                provider.increment();
                return true;
            case '\\':
                provider.increment();
                switch(provider.readCharacter()) {
                    case 'x':
                        consume_hex_digits(provider);
                        break;
                    case 'u':
                        if (provider.increment('{')) {
                            while (!provider.eof() && provider.peek() != '}') {
                                provider.increment();
                            }
                            provider.increment('}');
                        } else {
                            for (int i = 0; i < 4 && !provider.eof(); i++) {
                                provider.increment();
                            }
                        }
                        break;
                    default:
                        break;
                }
                continue;
            case '{':
                // does not consume the '{'
                return false;
            case '\0':
                return std::nullopt;
            default:
                provider.increment();
                continue;
        }
    }
}

// returns whether the ending quote was found
bool read_quoted_string(SourceProvider& provider) {
    while(true) {
        switch(provider.readCharacter()) {
            case '\\': {
                auto c = provider.readCharacter();
                if (c == 'x') {
                    consume_hex_digits(provider);
                } else if (c == 'u') {
                    if (provider.increment('{')) {
                        while (!provider.eof() && provider.peek() != '}') {
                            provider.increment();
                        }
                        provider.increment('}');
                    } else {
                        for (int i = 0; i < 4 && !provider.eof(); i++) {
                            provider.increment();
                        }
                    }
                }
                break;
            }
            case '"':
                return true;
            case '\n':
            case '\r':
            case '\0':
                return false;
            default:
                break;
        }
    }
}

bool read_char_in_quotes(SourceProvider& provider) {
    std::size_t len;
    auto cp = provider.utf8_decode_peek(len);
    if (cp == 0) return false;
    
    if (cp == '\\') {
        provider.incrementCodepoint(cp, len); // consume \
        cp = provider.utf8_decode_peek(len);
        if (cp == 0) return false;
        
        switch (cp) {
            case 'x':
                provider.incrementCodepoint(cp, len);
                consume_hex_digits(provider);
                break;
            case 'u':
                provider.incrementCodepoint(cp, len);
                if (provider.peek() == '{') {
                    provider.increment();
                    while (!provider.eof() && provider.peek() != '}') {
                        provider.increment();
                    }
                    if (provider.peek() == '}') provider.increment();
                } else {
                    for (int i = 0; i < 4 && !provider.eof(); i++) {
                        provider.increment();
                    }
                }
                break;
            default:
                provider.incrementCodepoint(cp, len);
                break;
        }
        return true;
    } else if (cp == '\'') {
        return false;
    } else {
        provider.incrementCodepoint(cp, len);
        return true;
    }
}

void read_id(SourceProvider& provider) {
    std::size_t len;
    while(true) {
        auto cp = provider.utf8_decode_peek(len);
        if(cp != 0u && isIdentifierContinue(cp)) {
            provider.incrementCodepoint(cp, len);
        } else {
            return;
        }
    }
}

void read_annotation_id(SourceProvider& provider) {
    std::size_t len;
    while(true) {
        const auto cp = provider.utf8_decode_peek(len);
        switch(cp) {
            case '_':
            case '.':
            case ':':
                provider.incrementCodepoint(cp, len);
                break;
            default:
                if(isIdentifierContinue(cp)) {
                    provider.incrementCodepoint(cp, len);
                } else {
                    return;
                }
                break;
            case '\0':
                return;
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
            CHEM_THROW_RUNTIME("unknown mode triggered");
#endif
        }
    }
    const auto curr_data_ptr = provider.current_data();
    const auto cp = provider.readCodePoint();
    switch(cp) {
        case '\0':
            return Token(TokenType::EndOfFile, { "", 0 }, pos);
        case '{':
            if(str_expr) {
                return Token(TokenType::StringExprStart, view_str(LBraceCStr), pos);
            } else {
                return Token(TokenType::LBrace, view_str(LBraceCStr), pos);
            }
        case '}':
            if(str_expr) {
                auto ended_ = read_backtick_string(provider);
                if(ended_.has_value()) {
                    if(ended_.value()) {
                        // string ended
                        str_expr = false;
                    } else {
                        // another expression began
                    }
                } else {
                    diagnoser.diagnostic("no ending backtick for expressive string", chem::string_view(file_path), pos, provider.position(), DiagSeverity::Error);
                    str_expr = false;
                }
                const auto end = provider.current_data() - (ended_.has_value() && ended_.value() ? 1 : 0);
                return Token(TokenType::StringExprEnd, chem::string_view(curr_data_ptr, end - curr_data_ptr), pos);
            } else {
                return Token(TokenType::RBrace, view_str(RBraceCStr), pos);
            }
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
        case '$':
            return Token(TokenType::DollarSym, view_str(DollarCStr), pos);
        case '?':
            return Token(TokenType::QuestionMarkSym, view_str(QuestionMarkCStr), pos);
        case '@': {
            const auto first = provider.readCodePoint();
            if(!isIdentifierStart(first)) {
                diagnoser.diagnostic(AnnotMacroStartErr, chem::string_view(file_path), pos, provider.position(), DiagSeverity::Error);
            }
            read_annotation_id(provider);
            return Token(TokenType::Annotation, view_from(provider, curr_data_ptr), pos);
        }
        case '#': {
            const auto first = provider.readCodePoint();
            if(!isIdentifierStart(first)) {
                diagnoser.diagnostic(AnnotMacroStartErr, chem::string_view(file_path), pos, provider.position(), DiagSeverity::Error);
            }
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
                if(keep_comments) {
                    provider.increment();
                    const auto data_ptr = provider.current_data();
                    read_current_line(provider);
                    return Token(TokenType::SingleLineComment, view_from(provider, data_ptr), pos);
                }
                read_current_line(provider);
                return getNextToken();
            } else if(p == '*') {
                if(keep_comments) {
                    provider.increment();
                    const auto start = provider.current_data();
                    const auto end = read_multi_line_comment_text(provider);
                    return Token(TokenType::MultiLineComment, chem::string_view(start, end - start), pos);
                }
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
        case '`': {
            switch(provider.peek()) {
                case '{':
                    provider.increment();
                    // string expression found
                    if(str_expr) {
                        diagnoser.diagnostic("nested string expressions aren't allowed", chem::string_view(file_path), pos, provider.position(), DiagSeverity::Error);
                    } else {
                        str_expr = true;
                    }
                    return Token(TokenType::StringExprStart, view_str(LBraceCStr), pos);
                default:
                    // single line string
                    const auto start = provider.current_data();
                    const auto ended = read_backtick_string(provider);
                    if(ended.has_value()) {
                        if(ended.value()) {
                            // found double quotes
                            str_expr = false;
                        } else {
                            // string expression found
                            if(str_expr) {
                                diagnoser.diagnostic("nested string expressions aren't allowed", chem::string_view(file_path), pos, provider.position(), DiagSeverity::Error);
                            } else {
                                str_expr = true;
                            }
                        }
                    } else {
                        diagnoser.diagnostic("no ending quotes for expressive string", chem::string_view(file_path), pos, provider.position(), DiagSeverity::Error);
                    }
                    const auto end = provider.current_data() - (ended.has_value() && ended.value() ? 1 : 0);
                    return Token(TokenType::BacktickString, chem::string_view(start, end - start), pos);
            }
        }
        case '"':
            switch(provider.peek()) {
                case '"':
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
                default:
                    // single line string
                    const auto start = provider.current_data();
                    const auto ended = read_quoted_string(provider);
                    if(!ended) {
                        diagnoser.diagnostic("no ending quotes for the single line string", chem::string_view(file_path), pos, provider.position(), DiagSeverity::Error);
                    }
                    const auto end = provider.current_data() - (ended ? 1 : 0);
                    return Token(TokenType::String, chem::string_view(start, end - start), pos);
            }
        case ' ':
        case '\t':
            // skip the whitespace
            provider.skipWhitespaces();
            if(lex_whitespace) {
                return Token(TokenType::Whitespace, { curr_data_ptr, (unsigned long long) (provider.current_data() - curr_data_ptr) }, pos);
            }
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
    if(is_ascii_digit(cp)) {
        read_number(provider);
        return Token(TokenType::Number, view_from(provider, curr_data_ptr), pos);
    } else if(isIdentifierStart(cp)) {
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
}