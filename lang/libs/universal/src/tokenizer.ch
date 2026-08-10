/**
 * Standalone JSX-aware tokenizer for the runtime `universal` package.
 *
 * Tokenizes a universal component source string (JS + JSX) into a sequence
 * of compiler::Token values, so the shared universal_parser (driven by the
 * compiler `Parser` interface) can be reused at runtime without the compiler
 * lexer. Replicates the JSX state machine from universal_cbi's getNextToken
 * (jsx_depth / in_jsx_tag / jsx_brace_count), minus the embedded-chemical
 * mode which only exists in the compiler.
 */
using namespace std;

public struct UniversalTokenizer {
    var src : std::string_view
    var pos : size_t
    var line : uint
    var character : uint

    // jsx state machine
    var jsx_depth : int = 0
    var in_jsx_tag : int = 0
    var jsx_brace_count : int = 0
    var tag_mode_stack : ubigint = 0
    var jsx_brace_stack : ubigint = 0
}

func (t : &mut UniversalTokenizer) peek() : char {
    if(t.pos >= t.src.size()) return '\0'
    return t.src.get(t.pos)
}

func (t : &mut UniversalTokenizer) advance() {
    if(t.pos >= t.src.size()) return
    const c = t.src.get(t.pos)
    t.pos += 1
    if(c == '\n') {
        t.line += 1
        t.character = 0
    } else {
        t.character += 1
    }
}

func (t : &mut UniversalTokenizer) position() : Position {
    return Position { line : t.line, character : t.character }
}

func (t : &mut UniversalTokenizer) skip_whitespaces() {
    while(true) {
        const c = t.peek()
        switch(c) {
            ' ', '\t', '\n', '\r' => {
                t.advance()
            }
            default => {
                return
            }
        }
    }
}

func (t : &mut UniversalTokenizer) read_identifier() {
    while(true) {
        const c = t.peek()
        if(c != '\0' && (isalnum(c as int) || c == '-' || c == '_' || c == '$')) {
            t.advance()
        } else {
            return
        }
    }
}

func (t : &mut UniversalTokenizer) read_digits() {
    while(true) {
        const c = t.peek()
        if(isdigit(c)) {
            t.advance()
        } else {
            return
        }
    }
}

func (t : &mut UniversalTokenizer) read_quoted(quote : char) {
    t.advance() // consume the opening quote
    while(true) {
        const c = t.peek()
        if(c == quote) {
            t.advance()
            return
        } else if(c == '\\') {
            t.advance()
            if(t.peek() != '\0') t.advance()
        } else if(c != '\0') {
            t.advance()
        } else {
            return
        }
    }
}

func (t : &mut UniversalTokenizer) read_template() {
    t.advance() // consume the opening backtick
    while(true) {
        const c = t.peek()
        if(c == '`') {
            t.advance()
            return
        }
        if(c == '\0') {
            return
        }
        if(c == '\\') {
            t.advance()
            if(t.peek() != '\0') t.advance()
            continue
        }
        t.advance()
    }
}

func (t : &mut UniversalTokenizer) slice_from(start : size_t) : std::string_view {
    return std::string_view(t.src.data() + start, t.pos - start)
}

func (t : &mut UniversalTokenizer) next_token() : Token {
    const position = t.position()
    const start = t.pos

    // JSX child mode: text until < or { or eof
    const is_child = t.jsx_depth > 0 && t.in_jsx_tag == 0 && t.jsx_brace_count == 0
    if(is_child) {
        const p = t.peek()
        if(p != '<' && p != '{' && p != '\0') {
            while(true) {
                const n = t.peek()
                if(n == '<' || n == '{' || n == '\0') {
                    break
                }
                t.advance()
            }
            return Token { type : JsTokenType.JSXText as int, value : t.slice_from(start), position : position }
        }
    }

    const c = t.peek()
    switch(c) {
        '\0' => {
            return Token { type : JsTokenType.EndOfFile as int, value : std::string_view(""), position : position }
        }
        '{' => {
            t.advance()
            if(t.jsx_depth > 0) {
                t.jsx_brace_count++
                t.tag_mode_stack = (t.tag_mode_stack << 1) | (t.in_jsx_tag as ubigint)
                t.in_jsx_tag = 0
            }
            return Token { type : JsTokenType.LBrace as int, value : std::string_view("{"), position : position }
        }
        '}' => {
            t.advance()
            if(t.jsx_depth > 0 && t.jsx_brace_count > 0) {
                t.jsx_brace_count--
                t.in_jsx_tag = (t.tag_mode_stack & 1) as int
                t.tag_mode_stack = t.tag_mode_stack >> 1
            }
            return Token { type : JsTokenType.RBrace as int, value : std::string_view("}"), position : position }
        }
        '(' => {
            t.advance()
            return Token { type : JsTokenType.LParen as int, value : std::string_view("("), position : position }
        }
        ')' => {
            t.advance()
            return Token { type : JsTokenType.RParen as int, value : std::string_view(")"), position : position }
        }
        '[' => {
            t.advance()
            return Token { type : JsTokenType.LBracket as int, value : std::string_view("["), position : position }
        }
        ']' => {
            t.advance()
            return Token { type : JsTokenType.RBracket as int, value : std::string_view("]"), position : position }
        }
        ';' => {
            t.advance()
            return Token { type : JsTokenType.SemiColon as int, value : std::string_view(";"), position : position }
        }
        ',' => {
            t.advance()
            return Token { type : JsTokenType.Comma as int, value : std::string_view(","), position : position }
        }
        ':' => {
            t.advance()
            return Token { type : JsTokenType.Colon as int, value : std::string_view(":"), position : position }
        }
        '.' => {
            t.advance()
            if(t.peek() == '.') {
                t.advance()
                if(t.peek() == '.') {
                    t.advance()
                    return Token { type : JsTokenType.ThreeDots as int, value : std::string_view("..."), position : position }
                }
            }
            return Token { type : JsTokenType.Dot as int, value : std::string_view("."), position : position }
        }
        '+' => {
            t.advance()
            if(t.peek() == '+') {
                t.advance()
                return Token { type : JsTokenType.PlusPlus as int, value : std::string_view("++"), position : position }
            } else if(t.peek() == '=') {
                t.advance()
                return Token { type : JsTokenType.PlusEqual as int, value : std::string_view("+="), position : position }
            }
            return Token { type : JsTokenType.Plus as int, value : std::string_view("+"), position : position }
        }
        '-' => {
            t.advance()
            if(t.peek() == '-') {
                t.advance()
                return Token { type : JsTokenType.MinusMinus as int, value : std::string_view("--"), position : position }
            } else if(t.peek() == '=') {
                t.advance()
                return Token { type : JsTokenType.MinusEqual as int, value : std::string_view("-="), position : position }
            }
            return Token { type : JsTokenType.Minus as int, value : std::string_view("-"), position : position }
        }
        '*' => {
            t.advance()
            if(t.peek() == '=') {
                t.advance()
                return Token { type : JsTokenType.StarEqual as int, value : std::string_view("*="), position : position }
            }
            return Token { type : JsTokenType.Star as int, value : std::string_view("*"), position : position }
        }
        '/' => {
            t.advance()
            if(t.peek() == '=') {
                t.advance()
                return Token { type : JsTokenType.SlashEqual as int, value : std::string_view("/="), position : position }
            } else if(t.peek() == '>') {
                // /> self-closing
                if(t.in_jsx_tag == 1) {
                    if(t.jsx_depth > 0) {
                        t.jsx_depth--
                        t.jsx_brace_count = (t.jsx_brace_stack & 0xFF) as int
                        t.jsx_brace_stack >>= 8
                    }
                }
                return Token { type : JsTokenType.Slash as int, value : std::string_view("/"), position : position }
            } else if(t.peek() == '/') {
                // single line comment
                t.advance()
                while(true) {
                    const next = t.peek()
                    if(next == '\n' || next == '\0') {
                        break
                    }
                    t.advance()
                }
                return t.next_token()
            } else if(t.peek() == '*') {
                // multi line comment
                t.advance()
                while(true) {
                    const next = t.peek()
                    if(next == '\0') break
                    if(next == '*') {
                        t.advance()
                        if(t.peek() == '/') {
                            t.advance()
                            break
                        }
                        continue
                    }
                    t.advance()
                }
                return t.next_token()
            }
            return Token { type : JsTokenType.Slash as int, value : std::string_view("/"), position : position }
        }
        '&' => {
            t.advance()
            if(t.peek() == '&') {
                t.advance()
                return Token { type : JsTokenType.LogicalAnd as int, value : std::string_view("&&"), position : position }
            }
            return Token { type : JsTokenType.BitwiseAnd as int, value : std::string_view("&"), position : position }
        }
        '|' => {
            t.advance()
            if(t.peek() == '|') {
                t.advance()
                return Token { type : JsTokenType.LogicalOr as int, value : std::string_view("||"), position : position }
            }
            return Token { type : JsTokenType.BitwiseOr as int, value : std::string_view("|"), position : position }
        }
        '?' => {
            t.advance()
            return Token { type : JsTokenType.Question as int, value : std::string_view("?"), position : position }
        }
        ' ', '\t', '\n', '\r' => {
            t.skip_whitespaces()
            return t.next_token()
        }
        '=' => {
            t.advance()
            if(t.peek() == '=') {
                t.advance()
                if(t.peek() == '=') {
                    t.advance()
                    return Token { type : JsTokenType.StrictEqual as int, value : std::string_view("==="), position : position }
                }
                return Token { type : JsTokenType.EqualEqual as int, value : std::string_view("=="), position : position }
            } else if(t.peek() == '>') {
                t.advance()
                return Token { type : JsTokenType.Arrow as int, value : std::string_view("=>"), position : position }
            }
            return Token { type : JsTokenType.Equal as int, value : std::string_view("="), position : position }
        }
        '!' => {
            t.advance()
            if(t.peek() == '=') {
                t.advance()
                if(t.peek() == '=') {
                    t.advance()
                    return Token { type : JsTokenType.StrictNotEqual as int, value : std::string_view("!=="), position : position }
                }
                return Token { type : JsTokenType.NotEqual as int, value : std::string_view("!="), position : position }
            }
            return Token { type : JsTokenType.Exclamation as int, value : std::string_view("!"), position : position }
        }
        '`' => {
            t.read_template()
            return Token { type : JsTokenType.String as int, value : t.slice_from(start), position : position }
        }
        '<' => {
            t.advance()
            if(t.peek() == '=') {
                t.advance()
                return Token { type : JsTokenType.LessThanEqual as int, value : std::string_view("<="), position : position }
            } else if(t.peek() == '<') {
                t.advance()
                return Token { type : JsTokenType.LeftShift as int, value : std::string_view("<<"), position : position }
            }

            // JSX heuristics
            const p = t.peek()
            var is_jsx = false
            var is_closing = false

            if(p == '/') {
                is_jsx = true
                is_closing = true
            } else if(p == '>') {
                is_jsx = true
            } else if(isalpha(p as int) || p == '_' || p == '$' || p == '{') {
                is_jsx = true
            }

            if(is_jsx) {
                if(is_closing) {
                    if(t.jsx_depth > 0) {
                        t.jsx_depth--
                        t.jsx_brace_count = (t.jsx_brace_stack & 0xFF) as int
                        t.jsx_brace_stack >>= 8
                    }
                } else {
                    t.jsx_brace_stack = (t.jsx_brace_stack << 8) | (t.jsx_brace_count as ubigint)
                    t.jsx_brace_count = 0
                    t.jsx_depth++
                }
                t.in_jsx_tag = 1
            }

            return Token { type : JsTokenType.LessThan as int, value : std::string_view("<"), position : position }
        }
        '>' => {
            t.advance()
            if(t.peek() == '=') {
                t.advance()
                return Token { type : JsTokenType.GreaterThanEqual as int, value : std::string_view(">="), position : position }
            } else if(t.peek() == '>') {
                t.advance()
                if(t.peek() == '>') {
                    t.advance()
                    return Token { type : JsTokenType.RightShiftUnsigned as int, value : std::string_view(">>>"), position : position }
                }
                return Token { type : JsTokenType.RightShift as int, value : std::string_view(">>"), position : position }
            }
            if(t.in_jsx_tag == 1) {
                t.in_jsx_tag = 0
            }
            return Token { type : JsTokenType.GreaterThan as int, value : std::string_view(">"), position : position }
        }
        '"', '\'' => {
            t.read_quoted(c)
            return Token { type : JsTokenType.String as int, value : t.slice_from(start), position : position }
        }
        '~' => {
            t.advance()
            return Token { type : JsTokenType.BitwiseNot as int, value : std::string_view("~"), position : position }
        }
        '^' => {
            t.advance()
            return Token { type : JsTokenType.BitwiseXor as int, value : std::string_view("^"), position : position }
        }
        default => {
            if(isalpha(c as int) || c == '_' || c == '$') {
                t.read_identifier()
                const val = t.slice_from(start)
                const hash = fnv1_hash_view(&val)
                switch(hash) {
                    comptime_fnv1_hash("var") => { return Token { type : JsTokenType.Var as int, value : val, position : position } }
                    comptime_fnv1_hash("const") => { return Token { type : JsTokenType.Const as int, value : val, position : position } }
                    comptime_fnv1_hash("let") => { return Token { type : JsTokenType.Let as int, value : val, position : position } }
                    comptime_fnv1_hash("state") => { return Token { type : JsTokenType.State as int, value : val, position : position } }
                    comptime_fnv1_hash("for") => { return Token { type : JsTokenType.For as int, value : val, position : position } }
                    comptime_fnv1_hash("while") => { return Token { type : JsTokenType.While as int, value : val, position : position } }
                    comptime_fnv1_hash("break") => { return Token { type : JsTokenType.Break as int, value : val, position : position } }
                    comptime_fnv1_hash("continue") => { return Token { type : JsTokenType.Continue as int, value : val, position : position } }
                    comptime_fnv1_hash("switch") => { return Token { type : JsTokenType.Switch as int, value : val, position : position } }
                    comptime_fnv1_hash("case") => { return Token { type : JsTokenType.Case as int, value : val, position : position } }
                    comptime_fnv1_hash("default") => { return Token { type : JsTokenType.Default as int, value : val, position : position } }
                    comptime_fnv1_hash("do") => { return Token { type : JsTokenType.Do as int, value : val, position : position } }
                    comptime_fnv1_hash("try") => { return Token { type : JsTokenType.Try as int, value : val, position : position } }
                    comptime_fnv1_hash("catch") => { return Token { type : JsTokenType.Catch as int, value : val, position : position } }
                    comptime_fnv1_hash("finally") => { return Token { type : JsTokenType.Finally as int, value : val, position : position } }
                    comptime_fnv1_hash("throw") => { return Token { type : JsTokenType.Throw as int, value : val, position : position } }
                    comptime_fnv1_hash("function") => { return Token { type : JsTokenType.Function as int, value : val, position : position } }
                    comptime_fnv1_hash("return") => { return Token { type : JsTokenType.Return as int, value : val, position : position } }
                    comptime_fnv1_hash("if") => { return Token { type : JsTokenType.If as int, value : val, position : position } }
                    comptime_fnv1_hash("else") => { return Token { type : JsTokenType.Else as int, value : val, position : position } }
                    comptime_fnv1_hash("true") => { return Token { type : JsTokenType.True as int, value : val, position : position } }
                    comptime_fnv1_hash("false") => { return Token { type : JsTokenType.False as int, value : val, position : position } }
                    comptime_fnv1_hash("null") => { return Token { type : JsTokenType.Null as int, value : val, position : position } }
                    comptime_fnv1_hash("undefined") => { return Token { type : JsTokenType.Undefined as int, value : std::string_view("undefined"), position : position } }
                    comptime_fnv1_hash("new") => { return Token { type : JsTokenType.New as int, value : val, position : position } }
                    comptime_fnv1_hash("async") => { return Token { type : JsTokenType.Async as int, value : val, position : position } }
                    comptime_fnv1_hash("await") => { return Token { type : JsTokenType.Await as int, value : val, position : position } }
                    comptime_fnv1_hash("this") => { return Token { type : JsTokenType.This as int, value : val, position : position } }
                    comptime_fnv1_hash("of") => { return Token { type : JsTokenType.Of as int, value : val, position : position } }
                    comptime_fnv1_hash("typeof") => { return Token { type : JsTokenType.Typeof as int, value : val, position : position } }
                    comptime_fnv1_hash("void") => { return Token { type : JsTokenType.Void as int, value : val, position : position } }
                    comptime_fnv1_hash("delete") => { return Token { type : JsTokenType.Delete as int, value : val, position : position } }
                    comptime_fnv1_hash("in") => { return Token { type : JsTokenType.In as int, value : val, position : position } }
                    comptime_fnv1_hash("instanceof") => { return Token { type : JsTokenType.InstanceOf as int, value : val, position : position } }
                    comptime_fnv1_hash("class") => { return Token { type : JsTokenType.Class as int, value : val, position : position } }
                    comptime_fnv1_hash("extends") => { return Token { type : JsTokenType.Extends as int, value : val, position : position } }
                    comptime_fnv1_hash("super") => { return Token { type : JsTokenType.Super as int, value : val, position : position } }
                    comptime_fnv1_hash("static") => { return Token { type : JsTokenType.Static as int, value : val, position : position } }
                    comptime_fnv1_hash("import") => { return Token { type : JsTokenType.Import as int, value : val, position : position } }
                    comptime_fnv1_hash("export") => { return Token { type : JsTokenType.Export as int, value : val, position : position } }
                    comptime_fnv1_hash("yield") => { return Token { type : JsTokenType.Yield as int, value : val, position : position } }
                    comptime_fnv1_hash("debugger") => { return Token { type : JsTokenType.Debugger as int, value : val, position : position } }
                    default => {
                        return Token { type : JsTokenType.Identifier as int, value : val, position : position }
                    }
                }
            } else if(isdigit(c)) {
                t.read_digits()
                return Token { type : JsTokenType.Number as int, value : t.slice_from(start), position : position }
            }
            t.advance()
            return Token { type : 0, value : std::string_view("unexpected"), position : position }
        }
    }
}

func (t : &mut UniversalTokenizer) tokenize() : std::vector<Token> {
    var tokens = std::vector<Token>()
    while(true) {
        const tok = t.next_token()
        tokens.push(tok)
        if(tok.type == JsTokenType.EndOfFile as int) {
            break
        }
    }
    return tokens
}
