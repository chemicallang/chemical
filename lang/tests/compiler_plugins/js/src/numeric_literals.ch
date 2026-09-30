/**
 * Numeric literals in a `#js` / `#globaljs` body.
 *
 * A REGRESSION, and one whose failure mode is the reason it is worth a file of its
 * own. `read_digits` reads DECIMAL digits only, so a radix prefix used to be
 * dropped and the rest of the literal re-lexed as an identifier:
 *
 *     var hex = 0x8000;      ->  var hex = 0;x8000;
 *
 * That is not a syntax error. `node --check` passes, the bundle loads, every test
 * that asserts on substrings still passes, and the statement assigns 0 and then
 * throws `ReferenceError: x8000 is not defined` — when it is executed, and only
 * then. It shipped inside Web/account's passkey client as `var CHUNK = x8000`,
 * where it was caught only by a WebView test that actually ran the function.
 *
 * So the assertion here is not "does it contain 0x8000" — the broken output
 * contains `x8000`, and a substring check would be satisfied by it. It is
 * "is it ONE statement with the literal intact", which is the only form that
 * distinguishes the two.
 */

@test
public func test_hexadecimal_literal_is_one_token(env : &mut TestEnv) {
    var page = HtmlPage()
    #js {
        var chunk = 0x8000;
    }
    string_equals(env, page.toStringJsOnly(), """var chunk = 0x8000;""");
}

@test
public func test_hexadecimal_literal_accepts_both_cases(env : &mut TestEnv) {
    // `0xdeadBEEF` is one literal, not `0xdead` followed by an identifier `BEEF`.
    // The upper-case digits are the case that catches a scanner that only accepts
    // `a`-`f`.
    var page = HtmlPage()
    #js {
        var a = 0xdeadBEEF;
        var b = 0XCAFE;
    }
    string_equals(env, page.toStringJsOnly(), """var a = 0xdeadBEEF;var b = 0XCAFE;""");
}

@test
public func test_binary_and_octal_literals_are_one_token(env : &mut TestEnv) {
    // Mangled identically by the same line, so fixed by the same fix.
    var page = HtmlPage()
    #js {
        var bin = 0b1010;
        var oct = 0o777;
    }
    string_equals(env, page.toStringJsOnly(), """var bin = 0b1010;var oct = 0o777;""");
}

@test
public func test_a_plain_decimal_literal_is_unchanged(env : &mut TestEnv) {
    // The control. Without it a fix that swallowed the leading digit, or that
    // over-consumed, would still satisfy the tests above for the wrong reason.
    var page = HtmlPage()
    #js {
        var plain = 32768;
        var zero = 0;
        var trailing = 100;
    }
    string_equals(env, page.toStringJsOnly(), """var plain = 32768;var zero = 0;var trailing = 100;""");
}

@test
public func test_a_zero_prefix_that_is_not_a_literal_still_lexes_as_zero(env : &mut TestEnv) {
    // `0` followed by something that is not a radix prefix is just the number 0 —
    // `0.5`-style input never reaches here, and `x` alone is an identifier. This
    // pins that the prefix handling did not make the lexer greedy about `0`.
    var page = HtmlPage()
    #js {
        var a = 0;
        var b = 0 + 1;
    }
    string_equals(env, page.toStringJsOnly(), """var a = 0;var b = 0 + 1;""");
}

// NOT COVERED HERE, AND DELIBERATELY SO
//
// Two literal forms still do not work in a `#js` body, and both fail LOUDLY — a
// parse error naming the line — rather than emitting something that parses and
// misbehaves. They are recorded here so the next person does not have to rediscover
// them, and so nobody reads this file as "numbers are fine now".
//
//   * DECIMAL FLOATS: `1.5`, `.5`, `1e10`. The outer Chemical parser, which scans
//     the macro body to find its extent, rejects them before the JS lexer sees
//     them: `expected identifier after dot` for `1.5`, `unexpected token in
//     expression` for `.5` and `2E-3`.
//
//   * NUMERIC SEPARATORS: `1_000_000`. Also the outer Chemical lexer's
//     `read_digits`, which accepts digits only. Fixing that one is not a lexer
//     line: the value parser then has to strip the separators before converting, so
//     it is a lexer change plus a value-parser change, in C++ rather than in
//     Chemical. Left alone deliberately — it is a loud failure, and it is not the
//     bug this file exists for.
//
// A loud failure is a fine outcome. The bug worth a regression test is the one that
// produces valid output of the wrong meaning, and that class is now closed.
