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

/// Minify `source`, or return it unchanged when `js_is_minifiable` says no.
public func minify_js(source : std::string_view) : std::string {
    var out = std::string()
    if(!js_is_minifiable(source)) {
        var unchanged = source
        out.append_view(&unchanged)
        return out
    }
    var state = MinState()
    js_emit_minified(source, &mut out, &mut state)
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
