// safe — additive text helpers for component authors.
//
// The component declarations themselves are compile-time macros and already
// have no runtime raw-pointer surface. This file adds `&str` helpers for the
// two runtime calls a component/app author makes directly:
//
//   * `theme_scope(selector, vars)` — the `&str` form of
//     `componentsThemeScope`;
//   * `escape_text` / `escape_attr` — escape untrusted text before embedding it
//     in component children or attributes (delegates to `page::safe`).

public namespace components {

public namespace safe {

using std::string;
using std::string_view;

// Build a scoped CSS custom-property block for `selector`.
public func theme_scope(selector : &std::str, vars : &std::str) : string {
    var selector_view = selector.to_view()
    var vars_view = vars.to_view()
    return componentsThemeScope(&selector_view, &vars_view)
}

// HTML-escape untrusted text for use as element content.
public func escape_text(value : &std::str) : string {
    return escape_html_view(value.to_view())
}

// HTML-escape untrusted text for use inside a quoted attribute value.
public func escape_attr(value : &std::str) : string {
    return escape_html_view(value.to_view())
}

} // namespace safe

} // namespace components
