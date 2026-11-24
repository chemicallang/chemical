// ===== Build-time minification (JS + CSS) =====
//
// The emitted bundle is machine-generated JavaScript: four-space indentation, one
// statement per line, and explanatory comments. None of that has to reach the
// browser, and removing it is the cheapest wire-byte win in the stack —
// `minify_js` takes a router/app bundle to ~55% of its bytes with no other change.
//
// The rules are deliberately conservative, because a minifier that changes meaning
// is worse than a large bundle:
//
//   * string and template literals are copied byte for byte (including anything
//     inside `${…}`, which is why an interpolated expression is not minified);
//   * a newline is kept wherever automatic semicolon insertion could depend on it.
//     This emitter writes no semicolons, so collapsing newlines blindly merges
//     statements: `a = []` + newline + `b = 0` would become `a = []b = 0`;
//   * a space is kept wherever dropping it could change a token (`return x`,
//     `a + +b`, `1 .toString()`);
//   * the first `/` in a position where a regular-expression literal could start
//     makes the whole call fall back to the original bytes. Telling a regex from a
//     division needs more context than is worth guessing at here, and a wrong
//     guess silently rewrites code; `js_is_minifiable` reports the fallback, so a
//     build can print it instead of wondering why one chunk is larger.
//
// Everything here is deterministic — the same input always produces the same
// bytes — so a content hash still names the output it describes.
//
// Timing: this is a build-time cost (`StaticSite.build()`), never per request.

// ── Shared helpers ──────────────────────────────────────────────────────────

func min_word_char(c : char) : bool {
    if(c >= 'a' && c <= 'z') { return true }
    if(c >= 'A' && c <= 'Z') { return true }
    if(c >= '0' && c <= '9') { return true }
    return c == '_' || c == '$'
}

func min_space_char(c : char) : bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 11 || c == 12
}

func min_view_eq(v : std::string_view, lit : std::string_view) : bool {
    if(v.size() != lit.size()) { return false }
    var i : size_t = 0
    while(i < v.size()) {
        if(v.at(i) != lit.at(i)) { return false }
        i = i + 1
    }
    return true
}

// ── JavaScript literal boundaries ───────────────────────────────────────────
// Each helper takes the index of the opening delimiter and returns the index just
// past the literal. `ok` goes false on an unterminated literal, which makes the
// caller fall back to the original bytes.

public struct MinSpan {
    var end : size_t
    var ok : bool
}

func min_skip_quoted(src : std::string_view, start : size_t, quote : char) : MinSpan {
    var i = start + 1
    var n = src.size()
    while(i < n) {
        var c = src.at(i)
        if(c == '\\') { i = i + 2; continue }
        if(c == quote) { return MinSpan { end : i + 1, ok : true } }
        i = i + 1
    }
    return MinSpan { end : n, ok : false }
}

// Code inside `${ … }`: strings, nested templates and comments are honoured, and
// the closing brace is matched by depth.
func min_skip_interpolation(src : std::string_view, start : size_t) : MinSpan {
    var i = start
    var n = src.size()
    var depth : size_t = 1
    while(i < n) {
        var c = src.at(i)
        if(c == '\\') { i = i + 2; continue }
        if(c == '"' || c == '\'') {
            var s = min_skip_quoted(src, i, c)
            if(!s.ok) { return MinSpan { end : n, ok : false } }
            i = s.end
            continue
        }
        if(c == '`') {
            var t = min_skip_template(src, i)
            if(!t.ok) { return MinSpan { end : n, ok : false } }
            i = t.end
            continue
        }
        if(c == '/' && i + 1 < n) {
            var d = src.at(i + 1)
            if(d == '/') {
                var j = i
                while(j < n && src.at(j) != '\n') { j = j + 1 }
                i = j
                continue
            }
            if(d == '*') {
                var j = i + 2
                while(j + 1 < n) {
                    if(src.at(j) == '*' && src.at(j + 1) == '/') { j = j + 2; break }
                    j = j + 1
                }
                if(j + 1 >= n && (n < 2 || src.at(n - 1) != '/' || src.at(n - 2) != '*')) {
                    return MinSpan { end : n, ok : false }
                }
                i = j
                continue
            }
        }
        if(c == '{') { depth = depth + 1 }
        else if(c == '}') {
            depth = depth - 1
            if(depth == 0) { return MinSpan { end : i + 1, ok : true } }
        }
        i = i + 1
    }
    return MinSpan { end : n, ok : false }
}

func min_skip_template(src : std::string_view, start : size_t) : MinSpan {
    var i = start + 1
    var n = src.size()
    while(i < n) {
        var c = src.at(i)
        if(c == '\\') { i = i + 2; continue }
        if(c == '`') { return MinSpan { end : i + 1, ok : true } }
        if(c == '$' && i + 1 < n && src.at(i + 1) == '{') {
            var e = min_skip_interpolation(src, i + 2)
            if(!e.ok) { return MinSpan { end : n, ok : false } }
            i = e.end
            continue
        }
        i = i + 1
    }
    return MinSpan { end : n, ok : false }
}

func min_skip_line_comment(src : std::string_view, start : size_t) : size_t {
    var i = start
    var n = src.size()
    while(i < n && src.at(i) != '\n') { i = i + 1 }
    return i
}

func min_skip_block_comment(src : std::string_view, start : size_t) : MinSpan {
    var i = start + 2
    var n = src.size()
    while(i + 1 < n) {
        if(src.at(i) == '*' && src.at(i + 1) == '/') {
            return MinSpan { end : i + 2, ok : true }
        }
        i = i + 1
    }
    return MinSpan { end : n, ok : false }
}

// ── Whitespace policy ───────────────────────────────────────────────────────

/// True when removing the whitespace between `a` and `b` would merge them into a
/// different token.
func min_needs_space(a : char, b : char) : bool {
    if(min_word_char(a) && min_word_char(b)) { return true }
    if(a == b && (a == '+' || a == '-')) { return true }
    if(a == '/' && (b == '/' || b == '*')) { return true }
    // `1 .toString()` is not `1.toString()`; a digit followed by a dot needs the
    // space unless the dot starts a fraction.
    if(a >= '0' && a <= '9' && b == '.') { return true }
    return false
}

/// True when a newline between `a` and `b` has to survive: the emitter writes no
/// semicolons, so a statement boundary depends on it.
func min_keeps_newline(a : char, b : char) : bool {
    // After an operator/punctuation an expression continues: no ASI can happen.
    if(a == '(' || a == ',' || a == '=' || a == '+' || a == '-' || a == '*' || a == '/' || a == '%' ||
       a == '&' || a == '|' || a == '^' || a == '!' || a == '~' || a == '?' || a == ':' ||
       a == '<' || a == '>' || a == '[' || a == '{' || a == ';') {
        return false
    }
    // Before a closing token or a continuation the newline cannot end a statement.
    if(b == ')' || b == ']' || b == '}' || b == ',' || b == ';' || b == ':' || b == '.' ||
       b == '=' || b == '+' || b == '-' || b == '*' || b == '/' || b == '%' || b == '&' ||
       b == '|' || b == '^' || b == '?' || b == ':' || b == '<' || b == '>' || b == '[') {
        return false
    }
    return true
}

/// Words after which a `/` starts a regular-expression literal rather than a
/// division.
func min_prefix_keyword(word : std::string_view) : bool {
    if(min_view_eq(word, "return")) { return true }
    if(min_view_eq(word, "typeof")) { return true }
    if(min_view_eq(word, "instanceof")) { return true }
    if(min_view_eq(word, "case")) { return true }
    if(min_view_eq(word, "in")) { return true }
    if(min_view_eq(word, "of")) { return true }
    if(min_view_eq(word, "new")) { return true }
    if(min_view_eq(word, "delete")) { return true }
    if(min_view_eq(word, "void")) { return true }
    if(min_view_eq(word, "throw")) { return true }
    if(min_view_eq(word, "else")) { return true }
    if(min_view_eq(word, "do")) { return true }
    if(min_view_eq(word, "yield")) { return true }
    if(min_view_eq(word, "await")) { return true }
    return false
}

// Token classes for the regex-vs-division question.
//   0 start          1 value (a `/` divides)
//   2 `}`            (ambiguous: treated as "a regex could start here" — safe)
//   3 prefix         (a `/` would start a regex literal)
func min_kind_after_char(kind : int, c : char) : int {
    if(c == ')' || c == ']') { return 1 }
    if(c == '}') { return 2 }
    if(min_word_char(c)) { return 1 }
    return 3
}

// ── JavaScript: can this be minified? ───────────────────────────────────────

/// True when `minify_js` will rewrite `source`. False means the source uses a
/// construct this minifier refuses to guess about (a regular-expression literal,
/// or an unterminated literal/comment), and the original bytes are kept.
public func js_is_minifiable(source : std::string_view) : bool {
    var i : size_t = 0
    var n = source.size()
    var kind : int = 0
    while(i < n) {
        var c = source.at(i)
        if(min_space_char(c)) { i = i + 1; continue }
        if(c == '/' && i + 1 < n) {
            var d = source.at(i + 1)
            if(d == '/') { i = min_skip_line_comment(source, i); continue }
            if(d == '*') {
                var b = min_skip_block_comment(source, i)
                if(!b.ok) { return false }
                i = b.end
                continue
            }
            // A `/` here could only be a regex literal when the previous token
            // cannot take a division.
            if(kind == 0 || kind == 2 || kind == 3) { return false }
            i = i + 1
            kind = 3
            continue
        }
        if(c == '"' || c == '\'') {
            var s = min_skip_quoted(source, i, c)
            if(!s.ok) { return false }
            i = s.end
            kind = 1
            continue
        }
        if(c == '`') {
            var t = min_skip_template(source, i)
            if(!t.ok) { return false }
            i = t.end
            kind = 1
            continue
        }
        if(min_word_char(c)) {
            var start = i
            while(i < n && min_word_char(source.at(i))) { i = i + 1 }
            var head = unsafe(source.data() + start)
            var span = i - start
            var word = unsafe(std::string_view(head, span))
            kind = 1
            if(min_prefix_keyword(word)) { kind = 3 }
            continue
        }
        if(c == '{') { kind = 3; i = i + 1; continue }
        kind = min_kind_after_char(kind, c)
        i = i + 1
    }
    return true
}

// ── JavaScript: emit ────────────────────────────────────────────────────────

/// Where the last emitted byte left the minifier. Carrying it across two calls is
/// what makes it safe to minify a buffer in pieces: without it, `5` at the end of
/// one piece and `function` at the start of the next would be written `5function`.
public struct MinState {
    var has_last : bool = false
    var last : char = ' '
    var pending : bool = false
    var pending_nl : bool = false
}

func js_emit_minified(source : std::string_view, out : &mut std::string, state : &mut MinState) {
    var i : size_t = 0
    var n = source.size()
    var pending = state.pending
    var pending_nl = state.pending_nl
    var last = state.last
    var has_last = state.has_last

    while(i < n) {
        var c = source.at(i)

        if(min_space_char(c)) {
            pending = true
            if(c == '\n' || c == '\r') { pending_nl = true }
            i = i + 1
            continue
        }
        if(c == '/' && i + 1 < n) {
            var d = source.at(i + 1)
            if(d == '/') {
                i = min_skip_line_comment(source, i)
                pending = true
                pending_nl = true
                continue
            }
            if(d == '*') {
                var b = min_skip_block_comment(source, i)
                i = b.end
                pending = true
                pending_nl = true
                continue
            }
        }

        // Everything from `start` to `end` is copied byte for byte.
        var start = i
        var end = i + 1
        if(c == '"' || c == '\'') {
            var s = min_skip_quoted(source, i, c)
            end = s.end
        } else if(c == '`') {
            var t = min_skip_template(source, i)
            end = t.end
        } else if(min_word_char(c)) {
            var j = i
            while(j < n && min_word_char(source.at(j))) { j = j + 1 }
            end = j
        }

        if(pending && has_last) {
            var first = source.at(start)
            if(pending_nl) {
                if(min_needs_space(last, first) || min_keeps_newline(last, first)) {
                    out.append('\n')
                    last = '\n'
                }
            } else if(min_needs_space(last, first)) {
                out.append(' ')
                last = ' '
            }
        }
        pending = false
        pending_nl = false

        unsafe { out.append_with_len(source.data() + start, end - start) }
        last = source.at(end - 1)
        has_last = true
        i = end
    }

    state.pending = pending
    state.pending_nl = pending_nl
    state.last = last
    state.has_last = has_last
}

/// Mangle every `function` definition in `source`, leaving everything else byte
/// for byte.
///
/// The page buffer written by `writeToDirectory` and the per-page chunk are one
/// text with several definitions in it, while a shared bundle is a table of unit
/// ranges. Scanning for definitions here means both paths get the same treatment
/// from the same rules: each definition is handed to `mangle_js_locals` whole, so
/// a nested closure's bindings are still inside the range being analysed.
public func mangle_js(source : std::string_view) : std::string {
    var out = std::string()
    var n = source.size()
    var copied : ubigint = 0
    var i : ubigint = 0
    while(i < n) {
        var c = source.at(i)
        if(c == '"' || c == '\'') {
            var s = min_skip_quoted(source, i, c)
            if(!s.ok) { break }
            i = s.end
            continue
        }
        if(c == '`') {
            var t = min_skip_template(source, i)
            if(!t.ok) { break }
            i = t.end
            continue
        }
        if(c == '/' && i + 1 < n && (source.at(i + 1) == '/' || source.at(i + 1) == '*')) { break }
        if(min_word_char(c)) {
            var start = i
            while(i < n && min_word_char(source.at(i))) { i = i + 1 }
            var word = min_slice(source, start, i - start)
            if(min_view_eq(word, "function")) {
                var end = min_function_block_end(source, start)
                if(end > start) {
                    if(start > copied) {
                        unsafe { out.append_with_len(source.data() + copied, start - copied) }
                    }
                    var block = min_slice(source, start, end - start)
                    var mangled = mangle_js_locals(block)
                    var mangled_view = mangled.to_view()
                    out.append_view(&mangled_view)
                    copied = end
                    i = end
                    continue
                }
            }
            continue
        }
        i = i + 1
    }
    if(copied < n) {
        unsafe { out.append_with_len(source.data() + copied, n - copied) }
    }
    return out
}

/// Index just past the `}` of the function whose `function` keyword starts at
/// `start`, or `start` when the definition cannot be located unambiguously (an
/// unterminated literal, or no body).
func min_function_block_end(source : std::string_view, start : ubigint) : ubigint {
    var n = source.size()
    var i = start
    // `function` [name] `(`
    while(i < n && min_word_char(source.at(i))) { i = i + 1 }
    while(i < n && source.at(i) == ' ') { i = i + 1 }
    while(i < n && min_word_char(source.at(i))) { i = i + 1 }
    while(i < n && source.at(i) == ' ') { i = i + 1 }
    if(i >= n || source.at(i) != '(') { return start }
    var depth : ubigint = 0
    while(i < n) {
        var c = source.at(i)
        if(c == '"' || c == '\'') {
            var s = min_skip_quoted(source, i, c)
            if(!s.ok) { return start }
            i = s.end
            continue
        }
        if(c == '`') {
            var t = min_skip_template(source, i)
            if(!t.ok) { return start }
            i = t.end
            continue
        }
        if(c == '(') { depth = depth + 1 }
        else if(c == ')') {
            depth = depth - 1
            if(depth == 0) { i = i + 1; break }
        }
        i = i + 1
    }
    while(i < n && source.at(i) == ' ') { i = i + 1 }
    if(i >= n || source.at(i) != '{') { return start }
    depth = 0
    while(i < n) {
        var c = source.at(i)
        if(c == '"' || c == '\'') {
            var s = min_skip_quoted(source, i, c)
            if(!s.ok) { return start }
            i = s.end
            continue
        }
        if(c == '`') {
            var t = min_skip_template(source, i)
            if(!t.ok) { return start }
            i = t.end
            continue
        }
        if(c == '{') { depth = depth + 1 }
        else if(c == '}') {
            depth = depth - 1
            if(depth == 0) { return i + 1 }
        }
        i = i + 1
    }
    return start
}

/// Minify `source`, or return it unchanged when `js_is_minifiable` says no.
public func minify_js(source : std::string_view) : std::string {
    var out = std::string()
    if(!js_is_minifiable(source)) {
        var unchanged = source
        out.append_view(&unchanged)
        return out
    }
    var state = MinState()
    var whitespace = std::string()
    js_emit_minified(source, &mut whitespace, &mut state)
    var ws_view = whitespace.to_view()
    return mangle_js(ws_view)
}

// ── JavaScript: local-identifier mangling ───────────────────────────────────
//
// Whitespace is the cheap half of minification. The other half is the names the
// generator had to invent: a component body keeps its Chemical source names
// (`props`, `classes`, `card`, `out`), and each appears several times in a
// definition that costs ~600 bytes.
//
// Renaming a JavaScript binding needs scope analysis, because the same bytes are a
// binding in one place and a property key in another (`props.size` names no
// variable). Rather than build a scope model, this pass uses the strongest
// per-unit rule available:
//
//   * only a **function definition unit** is touched — a range `plan_chunks` may
//     move out, so a renamed name can never be needed from another file;
//   * candidates are that unit's parameters and its `var`/`let`/`const` bindings,
//     so a global component name (referenced from HTML by `$_uc`) is never one;
//   * a candidate is refused when **any** occurrence of its bytes is a property
//     access (`a.name`) or a shorthand property (`{name}` / `{name: …}`), which is
//     what makes a plain whole-word rewrite correct: every occurrence left is a
//     variable reference;
//   * the replacement must appear **nowhere in the unit** as a word, so a rename
//     cannot capture a global the body also refers to;
//   * a unit containing a backtick is refused outright: an interpolated `${…}` is
//     code, and this pass does not descend into templates.
//
// Anything refused keeps its bytes — a larger bundle, never a different program.

/// The alphabet short names are drawn from, cheapest (one character) first.
func min_alpha() : std::string_view {
    return std::string_view("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ")
}

/// Length of the name `slot` stands for: 52 one-character names, then two.
func min_short_len(slot : ubigint) : ubigint {
    if(slot < 52) { return 1 }
    return 2
}

/// The `i`th character of the name `slot` stands for.
func min_short_char(slot : ubigint, i : ubigint) : char {
    var alpha = min_alpha()
    if(slot < 52) { return alpha.at(slot) }
    var n = slot - 52
    if(i == 0) { return alpha.at(n / 52) }
    return alpha.at(n % 52)
}

/// A word JavaScript already means something else by.
func min_is_keyword(name : std::string_view) : bool {
    if(min_view_eq(name, "var")) { return true }
    if(min_view_eq(name, "let")) { return true }
    if(min_view_eq(name, "const")) { return true }
    if(min_view_eq(name, "function")) { return true }
    if(min_view_eq(name, "return")) { return true }
    if(min_view_eq(name, "if")) { return true }
    if(min_view_eq(name, "else")) { return true }
    if(min_view_eq(name, "for")) { return true }
    if(min_view_eq(name, "while")) { return true }
    if(min_view_eq(name, "do")) { return true }
    if(min_view_eq(name, "break")) { return true }
    if(min_view_eq(name, "continue")) { return true }
    if(min_view_eq(name, "switch")) { return true }
    if(min_view_eq(name, "case")) { return true }
    if(min_view_eq(name, "default")) { return true }
    if(min_view_eq(name, "new")) { return true }
    if(min_view_eq(name, "delete")) { return true }
    if(min_view_eq(name, "typeof")) { return true }
    if(min_view_eq(name, "instanceof")) { return true }
    if(min_view_eq(name, "in")) { return true }
    if(min_view_eq(name, "of")) { return true }
    if(min_view_eq(name, "void")) { return true }
    if(min_view_eq(name, "throw")) { return true }
    if(min_view_eq(name, "try")) { return true }
    if(min_view_eq(name, "catch")) { return true }
    if(min_view_eq(name, "finally")) { return true }
    if(min_view_eq(name, "class")) { return true }
    if(min_view_eq(name, "extends")) { return true }
    if(min_view_eq(name, "super")) { return true }
    if(min_view_eq(name, "this")) { return true }
    if(min_view_eq(name, "null")) { return true }
    if(min_view_eq(name, "true") || min_view_eq(name, "false")) { return true }
    if(min_view_eq(name, "yield")) { return true }
    if(min_view_eq(name, "await")) { return true }
    if(min_view_eq(name, "with")) { return true }
    if(min_view_eq(name, "debugger")) { return true }
    if(min_view_eq(name, "enum")) { return true }
    if(min_view_eq(name, "implements")) { return true }
    if(min_view_eq(name, "interface")) { return true }
    if(min_view_eq(name, "package")) { return true }
    if(min_view_eq(name, "private")) { return true }
    if(min_view_eq(name, "protected")) { return true }
    if(min_view_eq(name, "public")) { return true }
    if(min_view_eq(name, "static")) { return true }
    return false
}

func min_digit(c : char) : bool {
    return c >= '0' && c <= '9'
}

/// Byte compare of `[start, start+length)` in `src` with `name`.
func min_bytes_eq(src : std::string_view, start : ubigint, length : ubigint, name : std::string_view) : bool {
    if(length != name.size()) { return false }
    var i : ubigint = 0
    while(i < length) {
        if(src.at(start + i) != name.at(i)) { return false }
        i = i + 1
    }
    return true
}

/// A view of the word at `[start, start+length)`. The view borrows `src`, so it
/// only lives as long as the source it was taken from.
func min_slice(src : std::string_view, start : ubigint, length : ubigint) : std::string_view {
    if(length == 0) { return std::string_view("") }
    return unsafe(std::string_view(src.data() + start, length))
}

/// True when the two-character name `a`, `b` (`b` unused when `len` is 1) occurs as
/// a whole word in `src`. Used to prove a replacement is free in the unit.
func min_src_has_short(src : std::string_view, a : char, b : char, len : ubigint) : bool {
    var n = src.size()
    var i : ubigint = 0
    while(i < n) {
        var c = src.at(i)
        if(c == '"' || c == '\'') {
            var s = min_skip_quoted(src, i, c)
            if(!s.ok) { return true }
            i = s.end
            continue
        }
        if(c == '`') {
            var t = min_skip_template(src, i)
            if(!t.ok) { return true }
            i = t.end
            continue
        }
        if(min_word_char(c)) {
            var start = i
            while(i < n && min_word_char(src.at(i))) { i = i + 1 }
            if(i - start == len && src.at(start) == a) {
                if(len == 1 || src.at(start + 1) == b) { return true }
            }
            continue
        }
        i = i + 1
    }
    return false
}

/// True when the unit starts with a `function` keyword: only a definition is
/// mangled, never the top-level code in a gap (shared by every page).
func min_is_function_unit(src : std::string_view) : bool {
    var i : ubigint = 0
    var n = src.size()
    while(i < n && min_space_char(src.at(i))) { i = i + 1 }
    const marker = std::string_view("function")
    if(i + marker.size() >= n) { return false }
    if(!min_bytes_eq(src, i, marker.size(), marker)) { return false }
    var after = i + marker.size()
    if(!min_space_char(src.at(after))) { return false }
    return true
}

struct MinBinding {
    var start : ubigint = 0
    var length : ubigint = 0
    var slot : ubigint = 0
    var renamed : bool = false
}

/// Queue `[start, start+length)` as a candidate unless the same bytes are already
/// queued (a name declared in two nested scopes is one candidate, so both
/// declarations and every use are renamed together — the shadowing is unchanged).
func min_push_binding(list : &mut std::vector<MinBinding>, src : std::string_view, start : ubigint, length : ubigint) {
    if(length < 2) { return }
    var word = min_slice(src, start, length)
    var i : ubigint = 0
    while(i < list.size()) {
        var b = unsafe(&*list.get_ptr(i))
        if(b.length == length && min_bytes_eq(src, b.start, b.length, word)) { return }
        i = i + 1
    }
    unsafe { list.push_back(MinBinding { start : start, length : length, slot : 0, renamed : false }) }
}

/// The leading word of one parameter segment is a binding (`...rest` has none, and a
/// `{destructured}` parameter is refused later as a shorthand property).
func min_push_param(list : &mut std::vector<MinBinding>, src : std::string_view, start : ubigint, end : ubigint) {
    var i = start
    while(i < end && src.at(i) == ' ') { i = i + 1 }
    if(i >= end || !min_word_char(src.at(i)) || min_digit(src.at(i))) { return }
    var s = i
    while(i < end && min_word_char(src.at(i))) { i = i + 1 }
    min_push_binding(list, src, s, i - s)
}

/// Mangle the parameters and `var`/`let`/`const` bindings of one function unit.
/// Returns a copy of the input when the unit is not a function, contains a
/// backtick, or made no safe rename.
public func mangle_js_locals(piece : std::string_view) : std::string {
    var out = std::string()
    var n = piece.size()
    var unchanged = piece
    if(!min_is_function_unit(piece)) {
        out.append_view(&unchanged)
        return out
    }
    // A template literal can hold code; this pass does not descend into it.
    if(piece.contains(&std::string_view("`"))) {
        out.append_view(&unchanged)
        return out
    }

    var bindings = std::vector<MinBinding>()

    // ── Parameters: the first `(`, split at depth-0 commas.
    var pstart : ubigint = 0
    var pfound : bool = false
    var i : ubigint = 0
    while(i < n) {
        var c = piece.at(i)
        if(c == '"' || c == '\'') {
            var s = min_skip_quoted(piece, i, c)
            if(!s.ok) { out.append_view(&unchanged); return out }
            i = s.end
            continue
        }
        if(c == '(') { pstart = i + 1; pfound = true; break }
        i = i + 1
    }
    if(pfound) {
        var depth : ubigint = 0
        var seg : ubigint = pstart
        var j : ubigint = pstart
        while(j < n) {
            var c = piece.at(j)
            if(c == '"' || c == '\'') {
                var s = min_skip_quoted(piece, j, c)
                if(!s.ok) { out.append_view(&unchanged); return out }
                j = s.end
                continue
            }
            if(c == '(' || c == '[' || c == '{') { depth = depth + 1; j = j + 1; continue }
            if(c == ')') {
                if(depth == 0) { min_push_param(&mut bindings, piece, seg, j); break }
                depth = depth - 1
                j = j + 1
                continue
            }
            if(c == ']' || c == '}') {
                if(depth > 0) { depth = depth - 1 }
                j = j + 1
                continue
            }
            if(c == ',' && depth == 0) {
                min_push_param(&mut bindings, piece, seg, j)
                seg = j + 1
                j = j + 1
                continue
            }
            j = j + 1
        }
    }

    // ── `var`/`let`/`const` bindings anywhere in the body.
    i = 0
    while(i < n) {
        var c = piece.at(i)
        if(c == '"' || c == '\'') {
            var s = min_skip_quoted(piece, i, c)
            if(!s.ok) { out.append_view(&unchanged); return out }
            i = s.end
            continue
        }
        if(min_word_char(c)) {
            var start = i
            while(i < n && min_word_char(piece.at(i))) { i = i + 1 }
            var word = min_slice(piece, start, i - start)
            var is_decl = min_view_eq(word, "var") || min_view_eq(word, "let") || min_view_eq(word, "const")
            if(is_decl) {
                var k = i
                while(k < n && piece.at(k) == ' ') { k = k + 1 }
                if(k < n && min_word_char(piece.at(k)) && !min_digit(piece.at(k))) {
                    var dstart = k
                    while(k < n && min_word_char(piece.at(k))) { k = k + 1 }
                    min_push_binding(&mut bindings, piece, dstart, k - dstart)
                }
            }
            continue
        }
        i = i + 1
    }

    if(bindings.size() == 0) {
        out.append_view(&unchanged)
        return out
    }

    // ── Pass 1: classify every occurrence.
    //
    // An occurrence after `.` is a property name, not a reference to a binding, so
    // it neither counts as a use nor blocks the rename — `props.size` says nothing
    // about the `size` binding, it just must not be rewritten when emitting. A
    // *shorthand* property (`{size}` / `{size: …}`) is the ambiguous case: the same
    // bytes are the key, so that binding keeps its name.
    var refs = std::vector<ubigint>()
    var usable = std::vector<ubigint>()
    var b : ubigint = 0
    while(b < bindings.size()) {
        unsafe { refs.push_back(0) }
        unsafe { usable.push_back(1) }
        b = b + 1
    }
    var prev : char = ' '
    var has_prev : bool = false
    i = 0
    while(i < n) {
        var c = piece.at(i)
        if(c == '"' || c == '\'') {
            var s = min_skip_quoted(piece, i, c)
            if(!s.ok) { out.append_view(&unchanged); return out }
            i = s.end
            has_prev = true
            prev = piece.at(i - 1)
            continue
        }
        if(min_word_char(c)) {
            var start = i
            while(i < n && min_word_char(piece.at(i))) { i = i + 1 }
            var length = i - start
            var word = min_slice(piece, start, length)
            if(min_is_keyword(word)) { has_prev = true; prev = piece.at(i - 1); continue }
            var next : char = ' '
            var nx = i
            while(nx < n && piece.at(nx) == ' ') { nx = nx + 1 }
            if(nx < n) { next = piece.at(nx) }
            var after_dot = has_prev && prev == '.'
            var shorthand = has_prev && (prev == '{' || prev == ',') && (next == '}' || next == ',' || next == ':')
            var k : ubigint = 0
            while(k < bindings.size()) {
                var bd = unsafe(&*bindings.get_ptr(k))
                if(bd.length == length && min_bytes_eq(piece, bd.start, bd.length, word)) {
                    if(shorthand) {
                        unsafe { *usable.get_ptr(k) = 0 }
                    } else if(!after_dot) {
                        var seen = unsafe(*refs.get_ptr(k))
                        unsafe { *refs.get_ptr(k) = seen + 1 }
                    }
                }
                k = k + 1
            }
            has_prev = true
            prev = piece.at(i - 1)
            continue
        }
        if(!min_space_char(c)) { has_prev = true; prev = c }
        i = i + 1
    }

    // ── Pass 2: hand out the shortest name that appears nowhere in the unit.
    var slot : ubigint = 0
    var renamed_any : bool = false
    b = 0
    while(b < bindings.size()) {
        var bd = unsafe(&mut *bindings.get_ptr(b))
        var ok = unsafe(*usable.get_ptr(b) == 1 && *refs.get_ptr(b) >= 2)
        if(ok) {
            while(slot < 2756) {
                var candidate_slot = slot
                slot = slot + 1
                var len = min_short_len(candidate_slot)
                if(!min_src_has_short(piece, min_short_char(candidate_slot, 0), min_short_char(candidate_slot, 1), len)) {
                    bd.slot = candidate_slot
                    bd.renamed = true
                    renamed_any = true
                    break
                }
            }
        }
        b = b + 1
    }
    if(!renamed_any) {
        out.append_view(&unchanged)
        return out
    }

    // ── Pass 3: rewrite the unit, replacing whole word tokens only.
    i = 0
    var emit_prev : char = ' '
    var emit_has_prev : bool = false
    while(i < n) {
        var c = piece.at(i)
        if(c == '"' || c == '\'') {
            var s = min_skip_quoted(piece, i, c)
            unsafe { out.append_with_len(piece.data() + i, s.end - i) }
            i = s.end
            emit_has_prev = true
            emit_prev = piece.at(i - 1)
            continue
        }
        if(min_word_char(c)) {
            var start = i
            while(i < n && min_word_char(piece.at(i))) { i = i + 1 }
            var length = i - start
            var replaced = false
            // A word after `.` is a property name: it is copied even when it matches
            // a renamed binding.
            if(!(emit_has_prev && emit_prev == '.')) {
                var k : ubigint = 0
                while(k < bindings.size()) {
                    var bd = unsafe(&*bindings.get_ptr(k))
                    if(bd.renamed && bd.length == length && min_bytes_eq(piece, bd.start, bd.length, min_slice(piece, start, length))) {
                        var len = min_short_len(bd.slot)
                        out.append(min_short_char(bd.slot, 0))
                        if(len > 1) { out.append(min_short_char(bd.slot, 1)) }
                        replaced = true
                        break
                    }
                    k = k + 1
                }
            }
            if(!replaced) {
                unsafe { out.append_with_len(piece.data() + start, length) }
            }
            emit_has_prev = true
            emit_prev = piece.at(i - 1)
            continue
        }
        if(!min_space_char(c)) { emit_has_prev = true; emit_prev = c }
        out.append(c)
        i = i + 1
    }
    return out
}


// ── CSS ─────────────────────────────────────────────────────────────────────

/// True when `minify_css` will rewrite `source` (an unterminated string or
/// comment makes it keep the original bytes).
public func css_is_minifiable(source : std::string_view) : bool {
    var i : size_t = 0
    var n = source.size()
    while(i < n) {
        var c = source.at(i)
        if(c == '/' && i + 1 < n && source.at(i + 1) == '*') {
            var b = min_skip_block_comment(source, i)
            if(!b.ok) { return false }
            i = b.end
            continue
        }
        if(c == '"' || c == '\'') {
            var s = min_skip_quoted(source, i, c)
            if(!s.ok) { return false }
            i = s.end
            continue
        }
        if(c == '\\') { i = i + 2; continue }
        i = i + 1
    }
    return true
}

func css_drops_space_after(c : char) : bool {
    return c == '{' || c == '}' || c == ';' || c == ':' || c == ',' || c == '>' || c == '('
}

func css_drops_space_before(c : char) : bool {
    return c == '{' || c == '}' || c == ';' || c == ':' || c == ',' || c == '>' || c == ')' || c == '!'
}

/// Minify CSS: comments and all insignificant whitespace go, the last `;` of a
/// block goes, and string literals and `url(…)` contents are left alone. CSS has
/// no automatic-semicolon problem, so newlines never have to survive.
public func minify_css(source : std::string_view) : std::string {
    var out = std::string()
    if(!css_is_minifiable(source)) {
        var unchanged = source
        out.append_view(&unchanged)
        return out
    }
    var i : size_t = 0
    var n = source.size()
    var has_last : bool = false
    var last : char = ' '
    var pending : bool = false
    // A `;` right before `}` is dropped, so it is held back until the next
    // non-space character is known.
    var held_semicolon : bool = false

    while(i < n) {
        var c = source.at(i)
        if(min_space_char(c)) { pending = true; i = i + 1; continue }
        if(c == '/' && i + 1 < n && source.at(i + 1) == '*') {
            var b = min_skip_block_comment(source, i)
            i = b.end
            pending = true
            continue
        }

        var start = i
        var end = i + 1
        if(c == '"' || c == '\'') {
            var s = min_skip_quoted(source, i, c)
            end = s.end
        } else if(c == '\\') {
            end = i + 2
        }

        if(held_semicolon) {
            if(c != '}') {
                out.append(';')
                last = ';'
                has_last = true
            }
            held_semicolon = false
        }

        if(pending && has_last) {
            var first = source.at(start)
            if(!css_drops_space_after(last) && !css_drops_space_before(first)) {
                out.append(' ')
                last = ' '
            }
        }
        pending = false

        if(c == ';') {
            held_semicolon = true
            i = end
            continue
        }
        unsafe { out.append_with_len(source.data() + start, end - start) }
        last = source.at(end - 1)
        has_last = true
        i = end
    }
    if(held_semicolon) { out.append(';') }
    return out
}
