// Regression tests for functional pseudo-classes in compound selectors.
//
// BUG 1 (this test FAILS today): `:root:not(.dark)` is emitted as
// `:root :not(.dark)`. css_cbi inserts a space before the functional
// pseudo-class, turning a compound selector into a descendant selector. That
// changes which elements match: `:root :not(.dark)` also matches <body> under
// <html class="dark">, so dark mode inherited light-theme custom properties
// (this is what produced the account app's white-on-white toast).
// Expected: `:not()` must stay attached to the preceding simple selector.
//
// BUG 2 (not expressible as a compiling test — recording it here so both are
// fixed together): a *type* selector followed by `:not(...)`, e.g.
//     a:not(.active) { color: red; }
// fails to parse at all:
//     [Parser] error: expected a semicolon after the property's value
// The css parser treats the type-selector + `:not()` compound as declarations.

@test
public func compound_not_selector_keeps_no_space(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        :root:not(.dark) {
            --bg: #ffffff;
        }
    }
    var got = page.toStringCssOnly();
    compl_css_equals(env, &got, ":root:not(.dark) { --bg:#ffffff; }");
}
