// parse_regressions.ch — valid JavaScript the `#js` parser used to REFUSE.
//
// WHY A SEPARATE MODULE FROM ../js
// -------------------------------
// Every case here is valid JavaScript that the parser once rejected at parse
// time, which failed the whole compilation unit and would have taken the
// runnable tests down with it. These constructs are now supported; the module is
// kept separate from ../js so parser-level (lexing/grammar) cases stay grouped
// and cannot hide the emitter's wrong-OUTPUT regressions in ../js.
//
// THE CONTRACT FOR THESE TESTS
// ----------------------------
// Each case asserts BOTH that the construct is accepted and that the emitted
// JavaScript is right — acceptance alone would be enough to reintroduce silent
// miscompilation, which is exactly what made these bugs expensive:
//   - a multi-declarator `var` produced a parse error whose caret landed on the
//     comma, reading like a stray character rather than a missing grammar rule;
//   - a regex literal (`/&/g`) lexed as `/` then `&` then `/`, so the enclosing
//     function was silently dropped from the emitted bundle and the build stayed green;
//   - an escaped quote (`\"`) inside a string ended the literal early, also silently.
// Do not weaken an expectation to match a future regression: the expectation is
// the specification.

using std::string;
using std::string_view;

// ── A multi-declarator `var` ───────────────────────────────────────────────
//
//     [Parser] error: unexpected token in expression at  …:576:21
//
// The caret was on the COMMA. `var` with several declarators is extremely common
// JavaScript; found in Web/timeline as `var body, next;`, which is now two
// statements. Emitted back as the single statement it was written as.
@test
public func a_multi_declarator_var_is_accepted(env : &mut TestEnv) {
    var page = HtmlPage()
    #js {
        var body = "hello";
        var next = "";
        var both = body, next;
    }
    string_equals(env, page.toStringJsOnly(), "var body = \"hello\";var next = \"\";var both = body, next;");
}

// ── A regex literal ────────────────────────────────────────────────────────
//
// Previously silent (no error at all, which is worse than a wrong one): the
// regex could not be scanned, so the failure surfaced as the enclosing function
// being ABSENT from the emitted bundle with a successful build.
//
//     $timeline_note_blocks_escape = function(value) {
//         var text = String(value);
//         return text.replace(/&/g, "&amp;");   // this line
//     }
//
// and `/app.js` simply had no `$timeline_note_blocks_escape` in it.
//
// `new RegExp("^[a-f0-9]{24}$", "i")` was always accepted because it is a plain
// string; the lexer now resolves `/` against division (`regex_allowed` state).
@test
public func a_regex_literal_is_accepted(env : &mut TestEnv) {
    var page = HtmlPage()
    #js {
        var cleaned = "a&b".replace(/&/g, "&amp;");
    }
    string_equals(env, page.toStringJsOnly(), "var cleaned = \"a&b\".replace(/&/g, \"&amp;\");");
}

// ── An escaped quote inside a string literal ────────────────────────────────
//
// The string reader used to stop at the first quote, so `\"` ended the literal
// early (`String.fromCharCode(34)` worked, `\"` did not). Also silent.
//
// Found in Web/timeline as `text.split("\"").join("&quot;")`.
@test
public func an_escaped_quote_in_a_string_literal_is_accepted(env : &mut TestEnv) {
    var page = HtmlPage()
    #js {
        var quoted = "say \"hi\"";
    }
    string_equals(env, page.toStringJsOnly(), "var quoted = \"say \\\"hi\\\"\";");
}

// ── A backslash escape in general ───────────────────────────────────────────
//
// The control for the two above, and the case that says the gap was BACKSLASH
// rather than "escapes" or "regex" specifically: `\n` inside a string is the most
// ordinary JavaScript there is. It shares the fix with the escaped quote — both
// are handled by the escape-aware JS string reader.
@test
public func a_backslash_n_escape_in_a_string_literal_is_accepted(env : &mut TestEnv) {
    var page = HtmlPage()
    #js {
        var joined = "a\nb";
    }
    string_equals(env, page.toStringJsOnly(), "var joined = \"a\\nb\";");
}
