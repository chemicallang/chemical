// Raw <script> content must be lexed as text (JS, not HTML) and emitted
// verbatim, without HTML entity escaping. Regression for the raw-script lexer
// mode added for #universal_test.

@test
public func script_content_with_angle_brackets_is_preserved(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <script>if(1 < 2) { document.title = "a > b"; }</script>
    }
    string_equals(env, page.toStringHtmlOnly(), "<script>if(1 < 2) { document.title = \"a > b\"; }</script>");
}

@test
public func script_content_is_not_entity_escaped(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <script>var s = "a & b";</script>
    }
    string_equals(env, page.toStringHtmlOnly(), "<script>var s = \"a & b\";</script>");
}

@test
public func script_with_attributes_is_preserved(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <script type="module">export const x = 1;</script>
    }
    string_equals(env, page.toStringHtmlOnly(), "<script type=\"module\">export const x = 1;</script>");
}
