// Regression tests for functional pseudo-classes in compound selectors.
//
// `:root:not(.dark)` must stay a single compound selector: `:not()` attaches
// to the preceding simple selector with no space. Previously the selector
// parser only advanced its adjacency cursor past the `:` of a pseudo-class
// (not past the whole `:name(...)`), so the following `:not(...)` looked
// whitespace-separated and became a descendant combinator:
// `:root :not(.dark)`. That changes which elements match — `:root :not(.dark)`
// also matches <body> under <html class="dark">, so dark mode inherited
// light-theme custom properties (this is what produced the account app's
// white-on-white toast). Expected: `:not()` must stay attached to the
// preceding simple selector.
//
// BUG 2 (not expressible as a compiling test — recording it here as a
// separate, still-open issue): a *type* selector followed by `:not(...)`, e.g.
//     a:not(.active) { color: red; }
// fails to parse at all:
//     [Parser] error: expected a semicolon after the property's value
// The css parser treats the type-selector + `:not()` compound as declarations.
// This is a lexer/initial-state problem (the leading type selector is lexed as
// a property name), independent of the compound-selector adjacency fix.

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
