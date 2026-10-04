// dialect_regressions.ch — valid CSS the `#css` parser REFUSES.
//
// THE CONTRACT FOR FIXING THESE
// -----------------------------
// Each case is valid CSS that a browser accepts, and each is currently a build
// ERROR rather than a silently dropped rule. The expectation is the specification:
// do not change it to match the parser's current behaviour, because a test that
// agrees with the bug protects nothing. Each should go green when the property's
// value parser grows the missing form.
//
// Every quoted error names a position that is NOT where the problem is, which is
// recorded per case because that is what makes these expensive: the message for
// case 1 lands on the second value of a four-value shorthand and reads as though
// the first were malformed.
//
// A silent no-op would have been worse than the error, and that is the point of
// the design this file is pushing on — see case 4.

using std::string;
using std::string_view;

// ── The four-value `border-width` shorthand ────────────────────────────────
//
//     [Parser] error: expected a semicolon after the property's value at …:2009:29
//     [Parser] error: unexpected token in nested rule body at …:2009:29
//     [Parser] error: failed to parse nested rule at …:2009:29
//
// Three errors for one declaration, and the first points at the SECOND value — so
// the message reads as though `0` were the malformed token.
//
// This is not academic. Web/timeline draws a checklist tick with exactly this
// declaration on a `::after` (two edges only), and could not: the checkbox is a
// filled square instead. The per-side longhands that would replace it are case 2.
@test
public func the_four_value_border_width_shorthand_is_accepted(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        border-width: 0 2px 2px 0;
    }
    css_equals(env, page.toStringCssOnly(), "border-width:0 2px 2px 0;");
}

// ── The per-side longhands ─────────────────────────────────────────────────
//
// The obvious workaround, and it is NOT known to work: no `border-*-width`
// longhand appears anywhere in Web/timeline's stylesheet, so there is no evidence
// the parser accepts one. Written here so the workaround stops being assumed and
// starts being known.
//
// If these are accepted, the checkbox tick does not need case 1 fixed first, which
// makes this the more valuable of the two to get working.
@test
public func the_per_side_border_width_longhands_are_accepted(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        border-top-width: 0;
        border-right-width: 2px;
        border-bottom-width: 2px;
        border-left-width: 0;
    }
    css_equals(env, page.toStringCssOnly(), "border-top-width:0;border-right-width:2px;border-bottom-width:2px;border-left-width:0;");
}

// ── Multi-value `padding` and `margin` shorthands ───────────────────────────
//
// The control for case 1, and it is here because if the four-value form of
// `border-width` is rejected while four values work for `padding`, then the gap
// is the PROPERTY TABLE and not "the parser cannot do four values" — a different
// bug with a different fix, in a different file.
//
// `padding: 0 2px 2px 0` is used in production stylesheets throughout this tree,
// so if THIS fails then case 1 is not about shorthands at all.
@test
public func the_four_value_padding_shorthand_is_accepted(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        padding: 0 2px 2px 0;
    }
    css_equals(env, page.toStringCssOnly(), "padding:0 2px 2px 0;");
}

// ── Three-value `border-width` ─────────────────────────────────────────────
//
// The narrowest form of the same question: if ONE value works and THREE do not,
// the parser is not counting values, it is reading a fixed arity per property.
//
// Deliberately written as its own case rather than folded into case 1, because
// knowing which arities are supported is what a fix needs and one failing case
// cannot say it.
@test
public func the_three_value_border_width_shorthand_is_accepted(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        border-width: 2px 2px 0;
    }
    css_equals(env, page.toStringCssOnly(), "border-width:2px 2px 0;");
}