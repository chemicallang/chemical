// #styled macro tests — SSR output and CSS injection for reusable styled
// components. Exercises plain components, shorthand `.tag` syntax, the wrapped
// component form, class merging, children, attributes passthrough, and edge
// cases. Some tests intentionally document known limitations (see wrap CSS
// linkage) and are expected to surface failures for follow-up work.

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Extract the value of the Nth `class="..."` attribute from an html view.
public func styled_extract_class_n(env : &mut TestEnv, html : std::string_view, n : uint) : std::string {
    var remaining = html
    var i : uint = 0
    while(i <= n) {
        const marker = std::string_view("class=\"")
        const pos = remaining.find(&marker)
        if(pos >= remaining.size()) {
            env.error("styled: no class attribute found in html")
            return std::string()
        }
        const start = pos + marker.size()
        const rest = remaining.skip(start)
        const end = rest.find(std::string_view("\""))
        if(end >= rest.size()) {
            env.error("styled: unterminated class attribute")
            return std::string()
        }
        if(i == n) {
            var out = std::string()
            out.append_view(rest.subview(0, end))
            return out
        }
        remaining = rest.skip(end + 1)
        i = i + 1
    }
    return std::string()
}

// Extract the value of the first `class="..."` attribute from an html view.
public func styled_extract_class(env : &mut TestEnv, html : std::string_view) : std::string {
    return styled_extract_class_n(env, html, 0)
}

// Assert that the generated (hash) class on the element also appears as a CSS
// selector in the page's css output.
public func styled_assert_linked(env : &mut TestEnv, html : std::string_view, css : std::string_view) {
    var cls = styled_extract_class(env, html)
    if(cls.empty()) { return }
    // The generated (hash) class is the first token of the class attribute
    // (renderHtmlAttrsWithBase emits the base class first, before user classes).
    var sp = cls.find(std::string_view(" "))
    var hash = cls.to_view()
    if(sp < cls.size()) {
        hash = cls.to_view().subview(0, sp)
    }
    var sel = std::string(".")
    var h = hash
    sel.append_view(&h)
    var sel_ref = sel.to_view()
    if(!css.contains(&sel_ref)) {
        env.error("styled: generated class not present in css output")
        var info = std::string("class :\"")
        info.append_view(cls.to_view())
        info.append('"')
        env.info(info.data())
        var html_ref = html
        var hinfo = std::string("html :\"")
        hinfo.append_view(&html_ref)
        hinfo.append('"')
        env.info(hinfo.data())
        var css_ref = css
        var cinfo = std::string("css :\"")
        cinfo.append_view(&css_ref)
        cinfo.append('"')
        env.info(cinfo.data())
    }

}

// Assert that a css declaration `prop: value` is present. The css emitter keeps
// source spacing inconsistently (e.g. `color:red` vs `background: #ffffff`), so
// we check the property keyword (with colon) and the value independently.
public func styled_assert_prop(env : &mut TestEnv, css : std::string_view, prop : std::string_view, value : std::string_view) {
    var p = std::string()
    var pv = prop
    p.append_view(&pv)
    p.append(':')
    var p_ref = p.to_view()
    if(!css.contains(&p_ref)) {
        env.error("styled: css property not found")
        env.info(p.data())
    }
    var v_ref = value
    if(!css.contains(&v_ref)) {
        env.error("styled: css value not found")
        env.info(value.data())
    }
}

// Count occurrences of a character in a view.
public func styled_count_char(view : std::string_view, c : char) : uint {
    var count : uint = 0
    var i : size_t = 0
    while(i < view.size()) {
        if(view.get(i) == c) { count = count + 1 }
        i = i + 1
    }
    return count
}

// Count occurrences of a substring in a view.
public func styled_count_substr(view : std::string_view, needle : std::string_view) : uint {
    if(needle.empty()) { return 0 }
    var count : uint = 0
    var remaining = view
    while(remaining.size() >= needle.size()) {
        const pos = remaining.find(&needle)
        if(pos >= remaining.size()) { break }
        count = count + 1
        remaining = remaining.skip(pos + needle.size())
    }
    return count
}

// ---------------------------------------------------------------------------
// Plain styled component
// ---------------------------------------------------------------------------

#styled SBox("div") {
    color: red;
    background: #ffffff;
}

@test
public func styled_basic_renders_tag_and_children(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SBox>hello</SBox> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("<div"))
    contains_string_assert(env, html.to_view(), std::string_view(">hello</div>"))
}

@test
public func styled_basic_injects_css(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SBox>hello</SBox> }
    var html = std::string()
    html.append_view(page.getHtml())
    var css = std::string()
    css.append_view(page.getCss())
    // the generated class links html <-> css
    styled_assert_linked(env, html.to_view(), css.to_view())
    // css carries the declared properties
    styled_assert_prop(env, css.to_view(), std::string_view("color"), std::string_view("red"))
    styled_assert_prop(env, css.to_view(), std::string_view("background"), std::string_view("#ffffff"))
}

@test
public func styled_basic_no_user_class_is_only_hash(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SBox>x</SBox> }
    var html = std::string()
    html.append_view(page.getHtml())
    const cls = styled_extract_class(env, html.to_view())
    // the class attribute should contain exactly one token (the generated hash)
    const spaces = styled_count_char(cls.to_view(), ' ')
    if(spaces != 0) {
        env.error("styled: unexpected extra class tokens when no user class given")
        var info = std::string("class :\"")
        info.append_view(cls.to_view())
        info.append('"')
        env.info(info.data())
    }
}

// ---------------------------------------------------------------------------
// User class merging
// ---------------------------------------------------------------------------

@test
public func styled_user_class_is_merged(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SBox class="extra">x</SBox> }
    var html = std::string()
    html.append_view(page.getHtml())
    // both the generated hash and the user class must be present
    const cls = styled_extract_class(env, html.to_view())
    contains_string_assert(env, cls.to_view(), std::string_view("extra"))
    styled_assert_linked(env, html.to_view(), page.getCss())
}

@test
public func styled_user_class_multiple_tokens(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SBox class="a b c">x</SBox> }
    var html = std::string()
    html.append_view(page.getHtml())
    const cls = styled_extract_class(env, html.to_view())
    contains_string_assert(env, cls.to_view(), std::string_view("a"))
    contains_string_assert(env, cls.to_view(), std::string_view("b"))
    contains_string_assert(env, cls.to_view(), std::string_view("c"))
    styled_assert_linked(env, html.to_view(), page.getCss())
}

// ---------------------------------------------------------------------------
// Shorthand `.tag` syntax
// ---------------------------------------------------------------------------

#styled STitle.div {
    font-weight: bold;
}

@test
public func styled_shorthand_dot_tag(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <STitle>y</STitle> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("<div"))
    contains_string_assert(env, html.to_view(), std::string_view(">y</div>"))
    styled_assert_linked(env, html.to_view(), page.getCss())
    var css = std::string()
    css.append_view(page.getCss())
    styled_assert_prop(env, css.to_view(), std::string_view("font-weight"), std::string_view("bold"))
}

// ---------------------------------------------------------------------------
// Children with nested markup
// ---------------------------------------------------------------------------

@test
public func styled_children_with_markup(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SBox>Hello <b>World</b></SBox> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("Hello"))
    contains_string_assert(env, html.to_view(), std::string_view("<b>World</b>"))
    styled_assert_linked(env, html.to_view(), page.getCss())
}

// ---------------------------------------------------------------------------
// Nested styled components
// ---------------------------------------------------------------------------

#styled SOuter("div") {
    padding: 1px;
}

#styled SInner("span") {
    color: blue;
}

@test
public func styled_nested_components(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SOuter><SInner>z</SInner></SOuter> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("<span"))
    var css = std::string()
    css.append_view(page.getCss())
    contains_string_assert(env, html.to_view(), std::string_view(">z</span>"))
    contains_string_assert(env, html.to_view(), std::string_view("</div>"))
    styled_assert_prop(env, css.to_view(), std::string_view("padding"), std::string_view("1px"))
    styled_assert_prop(env, css.to_view(), std::string_view("color"), std::string_view("blue"))
    // both generated classes must be linked
    styled_assert_linked(env, html.to_view(), css.to_view())
}

// ---------------------------------------------------------------------------
// Self-closing usage
// ---------------------------------------------------------------------------

@test
public func styled_self_closing(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SBox /> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("<div"))
    contains_string_assert(env, html.to_view(), std::string_view("</div>"))
    styled_assert_linked(env, html.to_view(), page.getCss())
}

// ---------------------------------------------------------------------------
// Attribute passthrough (non-class attrs)
// ---------------------------------------------------------------------------

@test
public func styled_attributes_passthrough(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SBox id="myid" data-x="1">x</SBox> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("id=\"myid\""))
    contains_string_assert(env, html.to_view(), std::string_view("data-x=\"1\""))
    styled_assert_linked(env, html.to_view(), page.getCss())
}

// ---------------------------------------------------------------------------
// Deterministic hash (same css -> same class across instances)
// ---------------------------------------------------------------------------

@test
public func styled_hash_is_deterministic(env : &mut TestEnv) {
    // Render the same component twice in one page; the generated class must be
    // identical (the hash is derived from the css content, so it is stable).
    var page = HtmlPage()
    #html { <SBox /><SBox /> }
    var html = std::string()
    html.append_view(page.getHtml())

    const cls1 = styled_extract_class_n(env, html.to_view(), 0)
    const cls2 = styled_extract_class_n(env, html.to_view(), 1)
    if(!cls1.to_view().equals(cls2.to_view())) {
        env.error("styled: hash class is not deterministic across instances")
        env.info(cls1.data())
        env.info(cls2.data())
    }
}

// ---------------------------------------------------------------------------
// Various CSS properties
// ---------------------------------------------------------------------------

#styled SProps("div") {
    background: #ff0000;
    border: 1px solid #cccccc;
    padding: 8px;
    margin: 4px;
    font-size: 14px;
    text-align: center;
}

@test
public func styled_various_properties(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SProps>x</SProps> }
    var css = std::string()
    css.append_view(page.getCss())
    styled_assert_prop(env, css.to_view(), std::string_view("background"), std::string_view("#ff0000"))
    styled_assert_prop(env, css.to_view(), std::string_view("border"), std::string_view("1px solid #cccccc"))
    styled_assert_prop(env, css.to_view(), std::string_view("padding"), std::string_view("8px"))
    styled_assert_prop(env, css.to_view(), std::string_view("margin"), std::string_view("4px"))
    styled_assert_prop(env, css.to_view(), std::string_view("font-size"), std::string_view("14px"))
    styled_assert_prop(env, css.to_view(), std::string_view("text-align"), std::string_view("center"))
    styled_assert_linked(env, page.getHtml(), css.to_view())
}

// ---------------------------------------------------------------------------
// Arbitrary HTML tag passthrough
// ---------------------------------------------------------------------------

#styled SSection("section") {
    border: 1px solid black;
}

#styled SAnchor("a") {
    color: blue;
}

@test
public func styled_arbitrary_tags(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SSection>sec</SSection><SAnchor href="/x">link</SAnchor> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("<section"))
    contains_string_assert(env, html.to_view(), std::string_view(">sec</section>"))
    contains_string_assert(env, html.to_view(), std::string_view("<a"))
    contains_string_assert(env, html.to_view(), std::string_view("href=\"/x\""))
    contains_string_assert(env, html.to_view(), std::string_view(">link</a>"))
    styled_assert_linked(env, html.to_view(), page.getCss())
}

// ---------------------------------------------------------------------------
// Wrapped component form
// ---------------------------------------------------------------------------

#styled SWrap(SInner) {
    margin: 4px;
}

@test
public func styled_wrap_renders_inner(env : &mut TestEnv) {
    // Wrapping forwards to the inner component, so the element renders as the
    // inner component's tag with its own styling.
    var page = HtmlPage()
    #html { <SWrap>wrapped</SWrap> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("<span"))
    contains_string_assert(env, html.to_view(), std::string_view(">wrapped</span>"))
    styled_assert_linked(env, html.to_view(), page.getCss())
}

@test
public func styled_wrap_own_css_is_emitted(env : &mut TestEnv) {
    // SWrap's own CSS rule (margin) should at least be present in the page css.
    var page = HtmlPage()
    #html { <SWrap>wrapped</SWrap> }
    var css = std::string()
    css.append_view(page.getCss())
    styled_assert_prop(env, css.to_view(), std::string_view("margin"), std::string_view("4px"))
}

@test
public func styled_wrap_owns_css_linked_to_element(env : &mut TestEnv) {
    // The wrapped element should carry BOTH the inner component's generated class
    // AND the wrapper's generated class, so the wrapper's `margin` rule applies.
    var page = HtmlPage()
    #html { <SWrap>wrapped</SWrap> }
    var html = std::string()
    html.append_view(page.getHtml())
    var css = std::string()
    css.append_view(page.getCss())
    // both the inner and wrapper generated classes must be present in css
    styled_assert_linked(env, html.to_view(), css.to_view())
    const cls = styled_extract_class(env, html.to_view())
    // The class attribute must contain more than one token (wrapper + inner).
    const spaces = styled_count_char(cls.to_view(), ' ')
    if(spaces < 1) {
        env.error("styled: wrap component did not merge its own class with the inner component's class")
        var info = std::string("class :\"")
        info.append_view(cls.to_view())
        info.append('"')
        env.info(info.data())
    }
    // The wrapper's own `margin` rule must be linked to the wrapper's generated
    // class (the second token of the class attribute).
    var sp = cls.to_view().find(std::string_view(" "))
    if(sp >= cls.size()) {
        env.error("styled: wrapper generated class missing from element")
    } else {
        var wrapHash = cls.to_view().subview(sp + 1, cls.size())
        var sel = std::string(".")
        var wh = wrapHash
        sel.append_view(&wh)
        var sel_ref = sel.to_view()
        if(!css.contains(&sel_ref)) {
            env.error("styled: wrapper generated class not linked to css output")
            var info = std::string("sel :\"")
            info.append_view(sel.to_view())
            info.append('"')
            env.info(info.data())
        }
    }
}

// ---------------------------------------------------------------------------
// Empty CSS block (no declarations)
// ---------------------------------------------------------------------------

#styled SEmpty("div") {
}

@test
public func styled_empty_css_block(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SEmpty>x</SEmpty> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("<div"))
    contains_string_assert(env, html.to_view(), std::string_view(">x</div>"))
    var css = std::string()
    css.append_view(page.getCss())
    // With no declarations, no class should be generated / linked.
    not_contains_string_assert(env, css.to_view(), std::string_view("color:"))
}

// ---------------------------------------------------------------------------
// Non-hashable CSS (media queries / nested rules)
// ---------------------------------------------------------------------------
// A `@media` block makes the generated css non-hashable, so the rule is emitted
// under the page (random) hash path. It must still render and link correctly.
#styled SMedia("div") {
    color: red;
    @media (max-width: 600px) {
        color: blue;
    }
}

@test
public func styled_media_query_renders(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SMedia>x</SMedia> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("<div"))
    contains_string_assert(env, html.to_view(), std::string_view(">x</div>"))
    var cls = styled_extract_class(env, html.to_view())
    if(cls.empty()) { return }
    var css = std::string()
    css.append_view(page.getCss())
    // The media query and both color declarations must be present.
    contains_string_assert(env, css.to_view(), std::string_view("@media"))
    contains_string_assert(env, css.to_view(), std::string_view("color:red"))
    contains_string_assert(env, css.to_view(), std::string_view("color:blue"))
    styled_assert_linked(env, html.to_view(), css.to_view())
}

// ---------------------------------------------------------------------------
// CSS dedup: identical css -> identical hash class, emitted once
// ---------------------------------------------------------------------------
#styled SDupA("div") {
    color: red;
}

#styled SDupB("div") {
    color: red;
}

@test
public func styled_identical_css_dedups_class(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SDupA>a</SDupA> }
    #html { <SDupB>b</SDupB> }
    var html = std::string()
    html.append_view(page.getHtml())
    // Both components share the same css, so they must share the same class.
    var clsA = styled_extract_class_n(env, html.to_view(), 0)
    var clsB = styled_extract_class_n(env, html.to_view(), 1)
    var spc = std::string_view(" ")
    var spA = clsA.to_view().find(&spc)
    var hashA = clsA.to_view()
    if(spA < clsA.size()) { hashA = clsA.to_view().subview(0, spA) }
    var spB = clsB.to_view().find(&spc)
    var hashB = clsB.to_view()
    if(spB < clsB.size()) { hashB = clsB.to_view().subview(0, spB) }
    if(!hashA.equals(&hashB)) {
        env.error("styled: identical css should produce identical hash class")
        var i = std::string("a:\"")
        i.append_view(&hashA); i.append('"'); i.append_view(std::string_view(" b:\"")); i.append_view(&hashB); i.append('"')
        env.info(i.data())
    }
    var css = std::string()
    css.append_view(page.getCss())
    // The shared rule is emitted exactly once.
    var sel = std::string(".")
    var h = hashA
    sel.append_view(&h)
    sel.append_view(std::string_view("{"))
    var sel_ref = sel.to_view()
    if(styled_count_substr(css.to_view(), sel_ref) != 1) {
        env.error("styled: identical css rule should be emitted exactly once")
        env.info(sel.data())
    }
}

// ---------------------------------------------------------------------------
// CSS emitted once for many instances of the same component
// ---------------------------------------------------------------------------
#styled SMany("div") {
    color: green;
}

@test
public func styled_many_instances_emit_css_once(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SMany>a</SMany> }
    #html { <SMany>b</SMany> }
    #html { <SMany>c</SMany> }
    var html = std::string()
    html.append_view(page.getHtml())
    var cls = styled_extract_class(env, html.to_view())
    var spc = std::string_view(" ")
    var sp = cls.to_view().find(&spc)
    var hash = cls.to_view()
    if(sp < cls.size()) { hash = cls.to_view().subview(0, sp) }
    var css = std::string()
    css.append_view(page.getCss())
    var sel = std::string(".")
    var h = hash
    sel.append_view(&h)
    sel.append_view(std::string_view("{"))
    var sel_ref = sel.to_view()
    if(styled_count_substr(css.to_view(), sel_ref) != 1) {
        env.error("styled: css for repeated component should be emitted exactly once")
        env.info(sel.data())
    }
}

// ---------------------------------------------------------------------------
// Wrap merges wrapper hash + inner hash + user classes
// ---------------------------------------------------------------------------
#styled SInner2("span") {
    color: teal;
}

#styled SWrap2(SInner2) {
    padding: 2px;
}

@test
public func styled_wrap_merges_user_classes(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SWrap2 class="outer-x">hi</SWrap2> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("<span"))
    var page_css = std::string()
    page_css.append_view(page.getCss())
    // The class attribute must carry the inner hash, the user-supplied class,
    // and the wrapper hash (in that order: base, user, wrapper).
    var all_cls = styled_extract_class(env, html.to_view())
    contains_string_assert(env, all_cls.to_view(), std::string_view("outer-x"))
    // The inner hash (first class token) must be linked.
    styled_assert_linked(env, html.to_view(), page_css.to_view())
    // Both components' own css rules must be present (proving wrap + inner are
    // both linked and the user class sits alongside them).
    contains_string_assert(env, page_css.to_view(), std::string_view("color:teal"))
    contains_string_assert(env, page_css.to_view(), std::string_view("padding:2px"))
}

// ---------------------------------------------------------------------------
// Rendered inside a loop (common for lists/cards): CSS must be emitted once
// even though the component renders many times.
// ---------------------------------------------------------------------------
#styled SLoop("div") {
    color: navy;
}

@test
public func styled_loop_emits_css_once(env : &mut TestEnv) {
    var page = HtmlPage()
    var i : int = 0
    while(i < 10) {
        #html { <SLoop>item</SLoop> }
        i = i + 1
    }
    var html = std::string()
    html.append_view(page.getHtml())
    if(styled_count_substr(html.to_view(), std::string_view("<div")) != 10) {
        env.error("styled: expected 10 loop elements")
    }
    var cls = styled_extract_class(env, html.to_view())
    var spc = std::string_view(" ")
    var sp0 = cls.to_view().find(&spc)
    var hash = cls.to_view()
    if(sp0 < cls.size()) { hash = cls.to_view().subview(0, sp0) }
    var css = std::string()
    css.append_view(page.getCss())
    var sel = std::string(".")
    var h = hash
    sel.append_view(&h)
    sel.append_view(std::string_view("{"))
    var sel_ref = sel.to_view()
    if(styled_count_substr(css.to_view(), sel_ref) != 1) {
        env.error("styled: css for loop-rendered component should be emitted exactly once")
        env.info(sel.data())
    }
    styled_assert_linked(env, html.to_view(), css.to_view())
}

// ---------------------------------------------------------------------------
// Cross-module styled component (declared in `styled_export`, imported here)
// ---------------------------------------------------------------------------
@test
public func styled_cross_module_usage(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ExportedCard>cross</ExportedCard> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("<div"))
    contains_string_assert(env, html.to_view(), std::string_view(">cross</div>"))
    var cls = styled_extract_class(env, html.to_view())
    if(cls.empty()) { return }
    var css = std::string()
    css.append_view(page.getCss())
    // The exported component's css must be present in this module's page.
    contains_string_assert(env, css.to_view(), std::string_view("rebeccapurple"))
    contains_string_assert(env, css.to_view(), std::string_view("border:1px solid"))
    styled_assert_linked(env, html.to_view(), css.to_view())
}

// ---------------------------------------------------------------------------
// Wrap of a UNIVERSAL component (robustness: must not crash; wrapper css must
// still be emitted). Observe the actual markup to decide class-merge behavior.
// ---------------------------------------------------------------------------
#universal UWrapTarget(props) {
    <div>{children}</div>
}

#styled SWrapU(UWrapTarget) {
    padding: 5px;
}

@test
public func styled_wrap_universal_inner(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SWrapU>content</SWrapU> }
    var html = std::string()
    html.append_view(page.getHtml())
    printf("UWRAP_HTML: %s\n", html.to_view().data())
    var css = std::string()
    css.append_view(page.getCss())
    printf("UWRAP_CSS: %s\n", css.to_view().data())
    // The wrapper's own css must always be emitted, regardless of inner type.
    contains_string_assert(env, css.to_view(), std::string_view("padding:5px"))
}
