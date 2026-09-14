---
name: Lexer and Tokens
description: Comprehensive guide to the Chemical lexer subsystem — how raw source bytes become a Token stream via Lexer/Lexer.cpp, the TokenType enum, LexUnit, SourceProvider scanning, literals/comments/whitespace handling, lexical diagnostics, the CBI user-lexer hooks, and the critical TokenType↔ChemicalTokenType enum-sync rule. Load when working on tokenization, adding a new token/keyword, debugging lexer errors, touching CBI macro lexing, or fixing enum-mismatch crashes in compiler plugins.
---

# Lexer and Tokens

The Chemical lexer is a hand-written, single-pass tokenizer. It converts raw source bytes into a flat
`std::vector<Token>` (wrapped in a `LexUnit`), which the parser then walks with a `Token*` cursor.
There is no regex/DFA generator and no separate token-stream class — the whole file is lexed up front.

## Architecture

### Pipeline

```
Source file → InputSource → SourceProvider (bytes + line/char tracking)
            → Lexer::getNextToken() loop → getTokens(std::vector<Token>&)
            → LexUnit { std::vector<Token> tokens }
            → Parser (Token* cursor) → AST
```

`ASTProcessor::parse_chemical_file` constructs the lexer, lexes the whole file, moves the lexer's
diagnostics out, aborts on lexical errors, then hands `tokens.data()` to the parser
(`compiler/ASTProcessor.cpp:1175-1222`):

```cpp
Lexer lexer(std::string(abs_path), *inp_source, &binder, file_allocator);
std::vector<Token> tokens;
lexer.getTokens(tokens);
// ... Unexpected handling ...
if(!lexer.diagnoser.diagnostics.empty()) {
    result.lex_diagnostics = std::move(lexer.diagnoser.diagnostics);
}
if(lexer.diagnoser.has_errors()) {
    result.continue_processing = false;
    return false;                    // do not parse on lexical error
}
Parser parser(fileId, abs_path, tokens.data(), loc_man, ..., &binder);
parser.parse(unit.scope.body.nodes);
```

### Key Files

| File | Purpose |
|------|---------|
| `lexer/Lexer.h` | `LexerState` + `Lexer` class declaration; flags, CBI hooks, entry points |
| `lexer/Lexer.cpp` | Full tokenizer implementation: `getNextToken`, scanning helpers, keyword map |
| `lexer/Token.h` | `Token` struct (kind, lexeme, position) + `isKeyword`/`isKeywordOrId` |
| `lexer/TokenType.h` | `TokenType` enum — the canonical C++ token kinds |
| `lexer/LexUnit.h` | `LexUnit` — a fully-lexed file's token vector |
| `lang/libs/compiler/src/ChemicalTokenType.ch` | Chemical-binding mirror of `TokenType` (CBI plugins) |
| `lang/libs/compiler/src/Token.ch` | Chemical-binding mirror of `Token` |
| `lang/libs/compiler/src/Lexer.ch` | Chemical-binding `Lexer` interface + user-lexer helpers |
| `stream/SourceProvider.h` | Byte cursor over `InputSource`, UTF-8 decode/peek, line/char tracking |
| `stream/InputSource.h`, `stream/FileInputSource.h` | In-memory / file-backed source buffers |
| `core/diag/Diagnoser.h` | `Diagnoser` used by the lexer (not `ASTDiagnoser`) |
| `core/diag/Position.h` | `Position { line, character }`, both zero-based |
| `compiler/cbi/bindings/LexerCBI.cpp` | CBI exports: file allocator, user lexer set/unset, embedded token |
| `compiler/cbi/model/Model.h` | `UserLexerGetNextToken`, `EmbeddedLexerInitializeFn` typedefs |
| `compiler/cbi/bindings/CBI.cpp` | Registers the `Lexer*` CBI symbols |
| `server/Importer.cpp` | LSP path: lexes for highlighting with `keep_comments` |

## TokenType Enum (`lexer/TokenType.h`)

`enum class TokenType` starts at `30000` so `0` is never a valid token (useful for zero-init
sentinels). Values are grouped in this exact order — order matters for the enum-sync rule:

```cpp
enum class TokenType {
    EndOfFile = 30000,            // TokenType.h:8
    Unexpected,                   // :9   lexer gave up / invalid char
    // ------- keywords -------
    ForKw, SwitchKw, LoopKw, ReturnKw, BreakKw, NewKw, ContinueKw, DestructKw,
    DeleteKw, DeallocKw, ProvideKw, DefaultKw, UnsafeKw, UnreachableKw, InitKw,
    ReversedKw, TryKw, CatchKw, ThrowKw, IfKw, FromKw, ElseKw, WhileKw, DoKw,
    TrueKw, FalseKw, NullKw, PublicKw, PrivateKw, ProtectedKw, InternalKw,
    I8Kw, I16Kw, I32Kw, I64Kw,               // chemical signed ints (:45)
    CharKw, ShortKw, IntKw, LongKw, LongLongKw, BigIntKw, Int128Kw,
    U8Kw, U16Kw, U32Kw, U64Kw,               // chemical unsigned ints (:60)
    UCharKw, UShortKw, UIntKw, ULongKw, ULongLongKw, UBigIntKw, UInt128Kw,
    BoolKw, AnyKw, DoubleKw, LongdoubleKw, FloatKw, Float128Kw, VoidKw,
    SizeOfKw, AlignOfKw, OffsetOfKw, RawKw, ImportKw, ExportKw, FuncKw, TypeKw,
    StructKw, UnionKw, VariantKw, InterfaceKw, ImplKw, NamespaceKw, EnumKw,
    AliasKw, VarKw, UsingKw, ComptimeKw, MutKw, SelfKw, ThisKw, AsKw, IsKw,
    InKw, DynKw, ZeroedKw, AsmKw, ConstKw, WhereKw,
    // -------
    Identifier, Whitespace, NewLine,          // :115-121
    LParen, RParen, LBrace, RBrace, LBracket, RBracket,  // :123-130
    Char, String, MultilineString, BacktickString, Lifetime, HashMacro, Annotation, // :134-146
    SingleLineComment, MultiLineComment,      // :149-152
    // symbols (:155-231)
    PlusSym, MinusSym, MultiplySym, DivideSym, ModSym,
    DoublePlusSym, DoubleMinusSym,
    EqualSym, DoubleEqualSym, NotEqualSym, LessThanOrEqualSym, LessThanSym,
    GreaterThanOrEqualSym, GreaterThanSym,
    LogicalAndSym, LogicalOrSym, LeftShiftSym, RightShiftSym,
    AmpersandSym, PipeSym, CaretUpSym, BitNotSym,
    ColonSym, DoubleColonSym, NotSym, DotSym, CommaSym, SemiColonSym,
    TripleDotSym, LambdaSym, DollarSym, QuestionMarkSym,
    Number,                                   // :236
    StringExprStart, StringExprEnd,           // :242-243  ${ ... } inside strings
    IndexKwStart = ForKw,                     // :245  keyword range sentinels
    IndexKwEnd = ConstKw,                     // :246
};
```

Naming conventions:
- Keywords end in `Kw` (`FuncKw`, `MutKw`); C-style and chemical type names are all keywords.
- Operator/punctuation tokens end in `Sym` (`PlusSym`, `DoubleColonSym`, `LambdaSym`, `TripleDotSym`).
- Multi-char operators are prefixed (`Double`, `Triple`, `LessThan`, `GreaterThan`, `Logical`, `LeftShift`).
- `Whitespace`, `NewLine`, `SingleLineComment`, `MultiLineComment` are **trivia** — emitted only in
  LSP/transformer modes (`lex_whitespace`/`keep_comments`), never in normal compilation.

`Token::isKeyword` uses the sentinels (`lexer/Token.h:43-45`):

```cpp
static inline bool isKeyword(enum TokenType type) {
    return type > TokenType::IndexKwStart && type < TokenType::IndexKwEnd;
}
static inline bool isKeywordOrId(enum TokenType type) {
    return type == TokenType::Identifier || isKeyword(type);
}
```

> ⚠️ The comparisons are **strict**. With `IndexKwStart = ForKw` and `IndexKwEnd = ConstKw`, the
> endpoints `ForKw`, `ConstKw`, and everything after `ConstKw` (i.e. `WhereKw`) are **not**
> reported as keywords by `isKeyword`. Verify these sentinels when touching keyword recognition.

## Token Representation (`lexer/Token.h:15-53`)

```cpp
struct Token {
    enum TokenType type;      // the token kind
    chem::string_view value;  // lexeme; for String/Char/backtick it excludes the quotes
    Position position;        // { line, character }, zero-based
#ifdef LSP_BUILD
    ASTAny* linked = nullptr; // node/value/type built from this token (LSP only)
#endif
};
```

- `position` is captured *before* reading the token (`Lexer.cpp:757`), so it is the start position.
- `value` for identifiers/numbers/strings is a **view into the source buffer** (see `view_from`,
  `Lexer.cpp:184-186`) — no copy. For symbols/keywords/newlines it points at a static C string
  literal (e.g. `view_str(PlusOpCStr)` at `Lexer.cpp:808`) or the keyword map key.
- `value.size()` is used by the parser to compute end positions (`Parser::end_pos`,
  `parser/Parser.cpp:43-45`) and single-token locations (`loc_single`, `parser/Parser.h:106-115`).
- `linked` exists only under `#ifdef LSP_BUILD`; the Chemical mirror guards it with `if(def.lsp)`
  in `lang/libs/compiler/src/Token.ch:10-12`.

## Lexer State (`lexer/Lexer.h`)

```cpp
struct LexerState {           // Lexer.h:26-48
    bool other_mode = false;  // delegated to a user lexer, or a non-normal mode
    bool user_mode  = false;  // a user-supplied lexer function is active
    bool str_expr   = false;  // currently inside an expressive-string ${ ... } expression
};

class Lexer : public LexerState {              // Lexer.h:53
    SourceProvider provider;                   // :59  byte cursor
    UserLexerGetNextToken user_lexer;          // :65  active user lexer (fn + instance)
    BatchAllocator& file_allocator;            // :71  exposed to user lexers
    std::vector<UserLexerGetNextToken> user_lexer_fns_stack; // :77  nesting stack
    std::string file_path;                     // :82  for diagnostics
    CompilerBinder* const binder;              // :88  CBI hook source; null = CBI disabled
    Diagnoser diagnoser;                       // :93  collects lexical diagnostics
    bool lex_whitespace = false;               // :100 keep Whitespace tokens (LSP/format)
    bool keep_comments  = false;               // :106 keep comment tokens (LSP/docs)

    Token getNextToken();                      // :124 one token
    void getTokens(std::vector<Token>&);       // :129 lex whole file
    void getUnit(LexUnit&);                    // :134 lex whole file into a LexUnit
};
```

The comment above `LexerState` states the design constraints: **all state options default to `false`**
and the state is intended to fit in a small bit-field (the comment mentions 9 booleans / a 32-bit
integer), though only three booleans are defined today.

`LexUnit` is a trivial wrapper (`lexer/LexUnit.h:13-27`):

```cpp
struct LexUnit {
    std::vector<Token> tokens;
    LexUnit() {}
};
```

## SourceProvider — the Byte Cursor (`stream/SourceProvider.h`)

The lexer never touches raw pointers directly; it drives a `SourceProvider` over an `InputSource`.

- `readCodePoint()` (`:176`) decodes a UTF-8 codepoint and advances; `utf8_decode_peek(len)` (`:105`)
  looks ahead without consuming.
- `peek()` (`:232`) returns the next `char` or `'\0'` at EOF; `peek_ahead(n)` (`:240`) looks further.
- `increment()` (`:204`) advances one byte; `increment(char c)` (`:258`) advances only if the next
  byte matches `c` (returns `bool`) — this is the workhorse of multi-char operator recognition.
- `handleCharacterRead`/`handleCodepointRead` (`:53`, `:67`) maintain `lineNumber` and
  `lineCharacterNumber` (both zero-based, incremented on `\n`, `\f`, and bare `\r`).
- `position()` (`:393`) returns `Position { line, character }`.
- `skipWhitespaces()` (`:313`) skips only `' '` and `'\t'` — **not** newlines.
- `eof()` (`:224`) checks `data_ == end_`.

## The Scanning Loop (`Lexer.cpp`)

`getNextToken` (`Lexer.cpp:756`) captures `pos`, then performs a single `switch` on the first
codepoint (`provider.readCodePoint()`), with fast paths for multi-char operators. The outer loop:

```cpp
void Lexer::getTokens(std::vector<Token>& tokens) {   // Lexer.cpp:1094
    tokens.reserve(250);
    while(true) {
        auto token = getNextToken();
        switch(token.type) {
            case TokenType::EndOfFile:
                tokens.emplace_back(token);           // parser needs a terminator
                return;
            case TokenType::Unexpected:
                tokens.emplace_back(token);
                return;                               // stop at the first unexpected token
            default:
                tokens.emplace_back(token);
        }
    }
}
void Lexer::getUnit(LexUnit& unit) { getTokens(unit.tokens); }   // :1112
```

- The default whitespace path skips `' '`/`'\t'` via `provider.skipWhitespaces()` and then
  **recurses** (`return getNextToken();`, `:1047`) unless `lex_whitespace` is set.
- Comments likewise recurse past the comment (`:913`, `:922`).
- `\0` (EOF) yields `EndOfFile` with an empty lexeme (`:772-773`).
- The final `default` fallback classifies digits/identifiers, otherwise emits `Unexpected` with
  the diagnostic `"unexpected token"` (`:1071-1072`).

There is a small amount of lookahead only through `provider.peek()`, `provider.increment(c)`, and
`provider.peek_ahead(n)` — there is no token-level backtracking in the lexer itself (the parser
re-lexes/backtracks by keeping a saved `Token*`, e.g. `Parser::isGenericEndAhead()`,
`parser/utils/Helpers.cpp:77-94`).

## Identifiers and Keywords

Identifiers are read with `read_id` (`Lexer.cpp:712-722`) using full UTF-8 codepoint predicates:

- `isIdentifierStart(cp)` (`:321-346`) is **blacklist-based**: ASCII letters/`_` fast path, then
  rejects surrogates, noncharacters, private-use, control/format chars, Unicode space separators,
  symbol/punctuation blocks, and digits; everything else is accepted.
- `isIdentifierContinue(cp)` (`:348-374`) is the same but also allows digits and explicitly allows
  ZWNJ/ZWJ (`U+200C`/`U+200D`).

After reading an identifier, the lexeme is looked up in a static `std::unordered_map<chem::string_view, TokenType>`
(`Lexer.cpp:62-182`). A hit becomes that keyword token; a miss becomes `Identifier`
(`Lexer.cpp:1061-1069`):

```cpp
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
```

Notes:
- The map is the authoritative keyword list; both `"self"` and `"Self"` map to `SelfKw`
  (`:170-171`), and `"f32"`/`"float"`, `"f64"`/`"double"` alias the same token
  (`:142-145`). Keyword token `value` is the map key (source spelling), not a normalized name.
- `read_annotation_id` (`:724-745`) additionally allows `_`, `.`, and `:` — used for
  `@module.sub::name` annotations and `#macro.name` hashes.

## Numeric Literals

Number scanning is split across small helpers:

| Helper | Location | Handles |
|--------|----------|---------|
| `read_digits` | `Lexer.cpp:376-385` | ASCII `0-9` |
| `read_alpha_or_digits` | `:387-396` | letters/digits (hex/octal bodies) |
| `read_floating_digits` | `:398-407` | digits, optional `.` + digits; returns whether a `.` was seen |
| `read_number_suffix` | `:409-439` | `i8`/`u16`/`ui32`/`uL`/`l`/`L`/`f` suffixes |
| `read_number` | `:444-453` | decimal/float; `f` suffix after a fractional part |
| `read_zero_starting_number` | `:458-477` | `0x`/`0X` hex, `0o`/`0O` octal, `0b`/`0B` binary |

Flow: the `case '0':` branch (`:1052-1054`) calls `read_zero_starting_number` so prefixes are
recognized; any other leading ASCII digit falls through to `read_number` (`:1058-1060`). The
negative sign is **never** part of the number — a leading `-` is a separate `MinusSym` and the
parser combines them. All numbers are emitted as `TokenType::Number` with the raw lexeme; actual
value conversion (including validating the suffix) happens later in
`Parser::parseNumberValue` → `convert_number_to_value` (`parser/utils/LexValue.cpp:316-324`).

## Strings, Chars, Backticks, Lifetimes

| Construct | Scanner | Token |
|-----------|---------|-------|
| `"..."` | `read_quoted_string` `:637-668` | `String` (lexeme excludes quotes) |
| `"""..."""` | `read_multi_line_string` `:517-543` | `MultilineString` |
| `` `...` `` | `read_backtick_string` `:593-634` | `BacktickString` |
| `'c'` / `'\x1b'` | `read_char_in_quotes` `:670-710` | `Char` (lexeme excludes quotes) |
| `'lifetime` | inline `:953-966` | `Lifetime` |
| `"a ${expr} b"` | `StringExprStart`/`StringExprEnd` | expressive string machinery |

- **Char vs Lifetime** (`:953-967`): after the opening `'`, if the next codepoint is an identifier
  start and the following char is another `'`, it is a one-character `Char`; if it is an identifier
  start followed by more identifier characters, it is a `Lifetime`. In all other cases
  `read_char_in_quotes` reads the (possibly escaped) character.
- **Escape sequences** are *not* decoded by the lexer — it only skips over them correctly so a
  quote inside an escape does not terminate the literal. `\x` consumes up to two hex digits
  (`consume_hex_digits`, `:506-515`); `\u{...}`/`\uXXXX` consumes the brace form or four hex
  digits. Decoding happens in `escaped_view` (`parser/utils/LexValue.cpp:160-186`).
- **Multiline strings** begin with three `"` and end at the next three `"`; unterminated strings
  end at EOF (`:536-538`).
- **Backtick strings** (`:978-1015`) are the lexing home of expressive strings:
  - a whole `` `hello` `` becomes one `BacktickString`;
  - on `` `${``, `read_backtick_string` returns "expression found" *without consuming* `${`;
    the lexer sets `str_expr = true` and emits `BacktickString`; the following `$` is re-entered
    at `case '$':` (`:847-858`), which consumes `{` and emits `StringExprStart`;
  - when the matching `}` is seen while `str_expr` is true (`:776-791`), the lexer re-enters
    `read_backtick_string` to consume the remainder and emits `StringExprEnd` whose lexeme is the
    trailing string segment; a following `${` emits another `StringExprStart`.
  - nested `${}` expressions are rejected with `"nested string expressions aren't allowed"`.

## Comments, Whitespace, Newlines

- `// ...` and `/* ... */` are recognized in the `/` branch (`Lexer.cpp:903-926`). In normal
  compilation they are silently skipped (recursion). With `keep_comments = true` they are emitted
  as `SingleLineComment` / `MultiLineComment` with the lexeme *after* the `//`/`/*` opener
  (`:906-910`, `:915-919`).
- `read_current_line` (`:479-488`) stops at `\n`/`\r`; `read_multi_line_comment_text`
  (`:490-501`) stops at `*/`.
- Spaces/tabs: skipped; emitted as `Whitespace` only when `lex_whitespace` is true (`:1040-1047`).
- `\n` → `NewLine` with lexeme `"\n"` (`:1048-1049`).
- `\r` → `win_new_line` (`:747-754`): a following `\n` produces lexeme `"\r\n"` (length 2),
  otherwise `"\r"` (length 1).
- The parser is expected to skip newlines explicitly via `BasicParser::consumeNewLines()`
  (`parser/Parser.cpp:110-118`), which only advances past `NewLine` tokens; the parser never sees
  `Whitespace`/comment tokens in normal mode.

## Annotations and Hash Macros (CBI)

- `@name` (`:861-868`): validates that the first char is an identifier start (error
  `"annotation or macro must start with a valid character that is not a digit"`), reads
  `read_annotation_id`, and emits `Annotation` with the lexeme **including** the leading `@`.
  The parser strips one char (`parser/statements/AnnotationMacro.cpp`).
- `#name` (`:869-886`): same validation; if `binder == nullptr`, emits an `Unexpected` token with
  `"unexpected '#' token, when no macro processing context exists"`. Otherwise it reads the name,
  looks up a `CBIFunctionType::InitializeLexer` hook via `binder->findHook(view, ...)`, and invokes
  it (`EmbeddedLexerInitializeFn`) before returning the `HashMacro` token (lexeme includes `#`).
  This is how a macro plugin installs a **custom lexer** for its embedded language (HTML/CSS/JS).

### User Lexer / Custom Lexing

`compiler/cbi/model/Model.h:36-58` defines the hook types; `LexerCBI.cpp` exports them:

```cpp
typedef void (*EmbeddedLexerInitializeFn)(Lexer* lexer);
typedef void (*EmbeddedLexerGetNextTokenFn)(Token* returning_token, void* instance, Lexer* lexer);
struct UserLexerGetNextToken { void* instance; EmbeddedLexerGetNextTokenFn subroutine; };
```

- `Lexer::getNextToken` dispatches to the user lexer when `other_mode && user_mode`
  (`Lexer.cpp:758-768`), returning whatever token the plugin produced.
- `LexersetUserLexer` / `LexerunsetUserLexer` (`LexerCBI.cpp:10-30`) set/restore
  `other_mode`/`user_mode`, pushing the previous user lexer on `user_lexer_fns_stack` for nesting.
- `LexergetEmbeddedToken` (`LexerCBI.cpp:32-43`) temporarily clears user mode to let the plugin
  read a *normal* token (e.g. to consume balanced `{ ... }` inside a macro body, as html_cbi does).
- `LexergetFileAllocator` exposes `Lexer::file_allocator` so plugin lexers can allocate stable
  strings on the file arena.

## Lexical Diagnostics

The lexer owns a plain `Diagnoser` (not `ASTDiagnoser`) and reports with the
`Position`-based API (`core/diag/Diagnoser.h:58`):

```cpp
diagnoser.diagnostic(message, chem::string_view(file_path), start_pos, end_pos, DiagSeverity::Error);
```

All lexer errors use `DiagSeverity::Error`, and each increments `Diagnoser::error_count`, so
`lexer.diagnoser.has_errors()` gates parsing (`ASTProcessor.cpp:1200-1208`). Lexer diagnostics are
moved into `ASTFileResult::lex_diagnostics` and printed by the normal diagnostics pipeline
(see the diagnostics skill). Representative messages:

| Message | Location | Trigger |
|---------|----------|---------|
| `no ending backtick for expressive string` | `Lexer.cpp:787` | `${}` unterminated |
| `unexpected dot symbol '.'` | `:837` | `..` without a third `.` |
| `annotation or macro must start with a valid character that is not a digit` | `:864`, `:872` | bad `@`/`#` |
| `unexpected '#' token, when no macro processing context exists` | `:875` | `#` with no binder |
| `no value given inside single quotes` | `:974` | `''` |
| `nested string expressions aren't allowed` | `:985`, `:1004` | `${` inside `${}` |
| `no ending quotes for expressive string` | `:1010` | unterminated backtick |
| `no ending quotes for the single line string` | `:1035` | unterminated `"` |
| `unexpected token` | `:1071` | unrecognized character |

`ASTProcessor` additionally rewrites the final `Unexpected` token into an
`unexpected token with value '...'` diagnostic (`ASTProcessor.cpp:1191-1197`).

## CRITICAL: Enum Sync Rule (`TokenType.h` ↔ `ChemicalTokenType.ch`)

> **When adding a new value to `lexer/TokenType.h`, you MUST add the same value, at the same
> position, to `lang/libs/compiler/src/ChemicalTokenType.ch`.**

| C++ Enum File | Chemical Binding File |
|---------------|-----------------------|
| `lexer/TokenType.h` | `lang/libs/compiler/src/ChemicalTokenType.ch` |
| `ast/base/ASTNodeKind.h` | `lang/libs/compiler/src/ast/base/ASTNodeKind.ch` |

**Why**: CBI plugins are compiled by TinyCC from Chemical source. They import the `compiler`
library, which contains `.ch` files whose enum values mirror the C++ enums. TCC-compiled plugin
code uses these Chemical enum values in comparisons and switches. If the C++ enum gains a value the
Chemical enum lacks, every subsequent value is **off by one** on the plugin side, so
`getKind()`/`token.type` comparisons take wrong branches, dereferencing garbage — a SIGSEGV in all
CBI plugins.

**Real bug**: adding `AsmKw` to `TokenType` in C++ without adding it to `ChemicalTokenType.ch`
shifted `RBrace` by one, which broke all `#html` macro parsing (the html parser closed on the wrong
brace token).

**Safe direction**: values appended at the **end** of the enum do not shift existing values and can
be added without immediate plugin breakage. Values inserted in the **middle** shift everything after
them and must be synced in the same change. The sentinels `IndexKwStart = ForKw` /
`IndexKwEnd = ConstKw` must also be kept identical in both files.

## Gotchas and Pitfalls

- **Token lexemes are non-owning views.** Identifier/number/string `value`s point into the
  `InputSource` buffer (or static literals for symbols/keywords). They stay valid only while the
  source buffer lives — copy into the AST arena (`Parser::loc_id` / `allocate_view`,
  `parser/Parser.cpp:47-51`) before the source is released.
- **The lexer does not decode escapes** — it only skips them. `\n` inside a string remains two
  bytes until `escaped_view` (`parser/utils/LexValue.cpp:160`). Unknown escapes are reported by the
  *parser*, not the lexer.
- **`Number` is lexed somewhat loosely** (e.g. `0o` bodies use `read_alpha_or_digits`), and suffix
  validation happens in the parser. A malformed numeric suffix fails at parse/typecheck time.
- **Whitespace/comments are opt-in.** LSP sets `lex_whitespace`/`keep_comments`
  (`server/Importer.cpp:34-49`); the normal parser is built assuming they are absent.
- **`str_expr` is lexer-global state.** Backtick templates toggle it and produce interleaved
  `BacktickString`/`StringExprStart`/`StringExprEnd` tokens; any new lexer path that can appear
  while `str_expr` is true must account for it. Nested templates are rejected.
- **The `case '$'` inside the backtick switch** (`Lexer.cpp:979-981`) checks
  `provider.peek() == '{'` while the switched value is already `'$'`, so the branch is effectively
  dead and control falls through to the single-line-string path. The real `${` handling is the
  top-level `case '$':` (`:847`) plus `read_backtick_string` returning `false`.
- **`isKeyword` endpoints are exclusive** (see warning above) — do not assume every `*Kw` is
  recognized by `isKeyword`.
- **`other_mode` without `user_mode`** only throws under `#ifdef DEBUG`
  (`Lexer.cpp:764-767`); release builds silently fall through the switch.
- **`getTokens` stops at the first `Unexpected`** token, so a single bad byte truncates the file's
  token stream. The parser is not run at all when `lexer.diagnoser.has_errors()` is true.

## Adding a New Token Type — Checklist

1. **`lexer/TokenType.h`** — add the enumerator in the correct group. Prefer appending to the end
   of the relevant group; never insert in the middle without updating the Chemical mirror.
2. **`lang/libs/compiler/src/ChemicalTokenType.ch`** — add the **identical** enumerator at the
   **same position** (keep `IndexKwStart`/`IndexKwEnd` correct).
3. **Lexer scanning** (`lexer/Lexer.cpp`) — emit the token from `getNextToken`. For an operator,
   use `provider.increment('c')` for lookahead; for a keyword, add an entry to the `keywords` map
   (`:62-182`); for a literal form, add a scanner helper and a `case` in the dispatch switch.
4. **Parser** (`parser/`) — teach the parser to consume/handle the new token (dispatch in
   `parser/statements/LexStatement.cpp`, expression ops in `parser/utils/Expression.cpp` +
   `ast/utils/Operation.cpp` precedence, or a dedicated construct parser).
5. **CBI plugins** — if the token is used inside macro parsing (e.g. `lang/libs/html_parser`),
   reference it as `ChemicalTokenType.<Name>` (values are exposed to Chemical as `int`, see
   `Token.ch:4` and `json_cbi/src/main.ch:7`).
6. **Tests** — add a lexer/parser test; for plugin-facing changes, compile a CBI plugin to confirm
   no enum mismatch (`./scripts/test.sh --tcc --plugins`).

## Related Skills

- **Parser Internals** (`.agents/skills/parser_internals/SKILL.md`) — how the parser consumes the
  token stream, including `consumeToken`/`consumeOfType`/`consumeNewLines`.
- **Diagnostics** (`.agents/skills/diagnostics/SKILL.md`) — `Diagnoser`/`Diag`/`Position` and how
  lexical diagnostics are printed.
- **Compiler Bindings** (`.agents/skills/compiler_bindings/SKILL.md`) — CBI overview and the
  canonical enum-sync rule.
- **Compiler Plugin API** (`.agents/skills/cbi_plugin_api/SKILL.md`) — writing macro plugins and the
  user-lexer hooks.
- **Compiler API** (`.agents/skills/compiler_api/SKILL.md`) — `lang/libs/compiler` bindings,
  including `Lexer.ch`, `Token.ch`, and `SourceProvider.ch`.
