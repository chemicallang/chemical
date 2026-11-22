// ============================================================================
// `<script>` content: raw text vs. interpolatable
// ============================================================================
// A `<script>` is lexed in a raw-text mode — everything up to `</script>` is one
// Text token, emitted verbatim with no HTML entity escaping — because JavaScript
// is not HTML and must not be lexed as HTML (`if (x) { ... }` is full of braces
// that are not chemical syntax).
//
// The justification is that the content *is* JavaScript, but the mode used to be
// keyed on the element NAME alone, so it also swallowed the one very common
// non-JS use of `<script>`: a data island.
//
//     <script type="application/json" id="r2" data-p={esc}></script>   WORKS
//     <script type="application/json" id="r3">{esc}</script>           WAS LITERAL
//
// Interpolation in attribute position worked on the same element, in the same
// block, so the rule that predicted the second line from the first said "yes"
// and the rule that predicted it from a plain `<div>{esc}</div>` said "no".
// Nothing was logged and the build reported zero errors: the page compiled,
// served, returned 200, and merely contained the seven characters `{esc}` where
// the payload should have been.
//
// The fix keys raw-text mode on the `type` attribute instead: raw text only
// where the content really is script data (no `type`, `module`, or a JavaScript
// MIME type). Every other type is a data block, so its content goes through the
// ordinary text/interpolation path like any other element.
//
// The tests below are in pairs: each "interpolates" test has a "stays raw"
// counterpart with the same payload shape, because the fix must not regress the
// JavaScript path that raw-text mode exists for.
//
// THE TRADE-OFF, stated plainly: inside a data-island script, '{' and '}' are
// now chemical syntax, exactly as they are in every other element of a #html
// block. A body written with literal JSON braces no longer parses -- but it
// fails LOUDLY, with a file/line, instead of silently emitting the wrong page,
// which is the failure mode this bug existed for. '<', '&' and '>' are ordinary
// text and are entity-escaped on output the same way they are in a <div>, which
// the browser decodes back. See script_data_island_body_is_ordinary_text.
// ============================================================================

// ------------------------------------------------------------ the bug itself

@test
public func script_json_island_interpolates_its_content(env : &mut TestEnv) {
    var payload = std::string("{\"uuid\":\"abc\"}")
    var page = HtmlPage()
    #html {
        <script type="application/json" id="r3">{payload.to_view()}</script>
    }
    // A `<script>` is a **raw text** element: the browser never entity-decodes its
    // content, so a value here is written verbatim. (Escaping it would put
    // `&quot;` inside the JSON and make it unparseable.) Not the literal source
    // text "{payload.to_view()}".
    string_equals(env, page.toStringHtmlOnly(),
        "<script type=\"application/json\" id=\"r3\">{\"uuid\":\"abc\"}</script>")
}

// The same payload through a <div>, pinning the equivalence the bug violated:
// a data-island script and any other element must interpolate identically.
@test
public func script_json_island_differs_from_div_interpolation(env : &mut TestEnv) {
    // The same payload in three positions, each with its own rule: element text is
    // escaped, an attribute is escaped, and a script body is raw text.
    var payload = std::string("{\"uuid\":\"abc\"}")
    var page = HtmlPage()
    #html {
        <div         id="r1">{payload.to_view()}</div>
        <script type="application/json" id="r2" data-p={payload.to_view()}></script>
        <script type="application/json" id="r3">{payload.to_view()}</script>
    }
    var expected = std::string("<div id=\"r1\">{&quot;uuid&quot;:&quot;abc&quot;}</div>")
    expected.append_view(std::string_view("<script type=\"application/json\" id=\"r2\" data-p=\"{&quot;uuid&quot;:&quot;abc&quot;}\"></script>"))
    expected.append_view(std::string_view("<script type=\"application/json\" id=\"r3\">{\"uuid\":\"abc\"}</script>"))
    string_equals(env, page.toStringHtmlOnly(), &expected.to_view())
}

// Other non-JS data types go down the same path, not just application/json.
@test
public func script_ld_json_island_interpolates_its_content(env : &mut TestEnv) {
    var payload = std::string("{\"@context\":\"https://schema.org\"}")
    var page = HtmlPage()
    #html {
        <script type="application/ld+json">{payload.to_view()}</script>
    }
    string_equals(env, page.toStringHtmlOnly(),
        "<script type=\"application/ld+json\">{\"@context\":\"https://schema.org\"}</script>")
}

// A plain data value interpolated into a data-island script, with no escaping
// ceremony: this is the shape products actually use for a server -> client
// payload handoff.
@test
public func script_data_island_interpolates_a_plain_value(env : &mut TestEnv) {
    var payload = std::string("hello island")
    var page = HtmlPage()
    #html {
        <script type="application/json">{payload.to_view()}</script>
    }
    string_equals(env, page.toStringHtmlOnly(),
        "<script type=\"application/json\">hello island</script>")
}

// Attribute interpolation on a data-island script must keep working (this is
// the sibling-product workaround that hid the bug; it must not regress).
// The quotes ARE entity-escaped: an attribute is an attribute, whatever element
// it is on, so `#html` escapes the value once (see escape.ch).
@test
public func script_data_island_attribute_interpolation_is_unaffected(env : &mut TestEnv) {
    var payload = std::string("{\"a\":1}")
    var page = HtmlPage()
    #html {
        <script type="application/json" data-p={payload.to_view()}></script>
    }
    string_equals(env, page.toStringHtmlOnly(),
        "<script type=\"application/json\" data-p=\"{&quot;a&quot;:1}\"></script>")
}

// Characters a JSON payload actually contains. A `<script>` is a raw text element
// (regardless of its `type`): '<' and '&' are ordinary text and the browser does
// NOT decode entities there, so escaping them would change the bytes the client
// parses. Literal text and interpolated values are both emitted verbatim.
@test
public func script_data_island_body_is_raw_text(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <script type="application/json">a < b && c > d</script>
    }
    string_equals(env, page.toStringHtmlOnly(),
        "<script type=\"application/json\">a < b && c > d</script>")
}

// ------------------------- what must NOT regress: the JavaScript raw path

// No `type` attribute at all: the content is JavaScript, so it stays raw and
// '{' is not chemical syntax.
@test
public func script_without_type_stays_raw_text(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <script>if(1 < 2) { document.title = "a > b"; }</script>
    }
    string_equals(env, page.toStringHtmlOnly(),
        "<script>if(1 < 2) { document.title = \"a > b\"; }</script>")
}

// type="module" is JavaScript, so it stays raw (this is the shape
// script_with_attributes_is_preserved in script_raw.ch already pins).
@test
public func script_module_stays_raw_text(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <script type="module">const o = { a: 1 }; if(o.a) { run(); }</script>
    }
    string_equals(env, page.toStringHtmlOnly(),
        "<script type=\"module\">const o = { a: 1 }; if(o.a) { run(); }</script>")
}

// An explicit JavaScript MIME type stays raw. The type match is
// ASCII case-insensitive, as MIME types are.
@test
public func script_javascript_mime_type_stays_raw_text(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <script type="text/javascript">if(1 < 2) { go(); }</script>
    }
    string_equals(env, page.toStringHtmlOnly(),
        "<script type=\"text/javascript\">if(1 < 2) { go(); }</script>")
}

@test
public func script_javascript_mime_type_is_case_insensitive(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <script type="TEXT/JavaScript">if(1 < 2) { go(); }</script>
    }
    string_equals(env, page.toStringHtmlOnly(),
        "<script type=\"TEXT/JavaScript\">if(1 < 2) { go(); }</script>")
}

// An empty `type` is not a data type, so it stays raw — a browser treats it as
// JavaScript.
@test
public func script_empty_type_stays_raw_text(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <script type="">if(1 < 2) { go(); }</script>
    }
    string_equals(env, page.toStringHtmlOnly(),
        "<script type=\"\">if(1 < 2) { go(); }</script>")
}

// Script-adjacent JSON types that browsers also treat as script data must stay
// raw, otherwise routing them through the interpolating path would make every
// '{' in an import map a chemical expression.
@test
public func script_importmap_type_stays_raw_text(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <script type="importmap">{"imports":{"lib":"/lib.js"}}</script>
    }
    string_equals(env, page.toStringHtmlOnly(),
        "<script type=\"importmap\">{\"imports\":{\"lib\":\"/lib.js\"}}</script>")
}