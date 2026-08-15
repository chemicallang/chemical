// Components library tests — SSR output and CSS registration for the
// shadcn-style components (Button, Badge, Alert, Card, Typography, Avatar,
// Toggle, Input, Utilities).

// Helper: assert `str` contains `needle`
public func contains_assert(env : &mut TestEnv, str : std::string_view, needle : std::string_view) {
    var needle_ref = needle
    var str_ref = str
    if(str_ref.contains(&needle_ref)) {
        return;
    }
    env.error("substring not found");
    var exp = std::string("needle :\"");
    exp.append_view(&needle_ref)
    exp.append('"');
    env.info(exp.data())
    var got = std::string("in     :\"");
    got.append_view(&str_ref)
    got.append('"');
    env.info(got.data())
}

public func contains_string_assert(env : &mut TestEnv, str : std::string_view, needle : std::string_view) {
    contains_assert(env, str, needle)
}

// ---------------------------------------------------------------------------
// Button: variant + size props drive data-variant / data-size attributes in SSR
// ---------------------------------------------------------------------------

@test
public func components_button_variant_default(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <Button>Save</Button> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("<button"))
    contains_string_assert(env, html.to_view(), std::string_view("data-variant=\"default\""))
    contains_string_assert(env, html.to_view(), std::string_view("type=\"button\""))
    contains_string_assert(env, html.to_view(), std::string_view(">Save</button>"))
    // CSS registered with the page
    var css = std::string()
    css.append_view(page.getCss())
    contains_string_assert(env, css.to_view(), std::string_view("[data-variant=\"default\"]"))
}

@test
public func components_button_variant_destructive(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <Button variant="destructive">Delete</Button> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("data-variant=\"destructive\""))
    contains_string_assert(env, html.to_view(), std::string_view(">Delete</button>"))
    var css = std::string()
    css.append_view(page.getCss())
    contains_string_assert(env, css.to_view(), std::string_view("[data-variant=\"destructive\"]"))
}

@test
public func components_button_size_sm(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <Button size="sm">Small</Button> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("data-size=\"sm\""))
    contains_string_assert(env, html.to_view(), std::string_view(">Small</button>"))
    var css = std::string()
    css.append_view(page.getCss())
    contains_string_assert(env, css.to_view(), std::string_view("[data-size=\"sm\"]"))
}

@test
public func components_button_className_merge(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <Button className="custom-class">Merged</Button> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("custom-class"))
    contains_string_assert(env, html.to_view(), std::string_view(">Merged</button>"))
}

@test
public func components_button_legacy_wrapper(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ButtonDanger>Legacy</ButtonDanger> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("data-variant=\"destructive\""))
    contains_string_assert(env, html.to_view(), std::string_view(">Legacy</button>"))
}

@test
public func components_button_loading(env : &mut TestEnv) {
    // loading swaps children for "Loading...", disables and sets aria-busy.
    var page = HtmlPage()
    #html { <Button loading={true}>Save</Button> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view(">Loading...</button>"))
    contains_string_assert(env, html.to_view(), std::string_view("disabled=\"true\""))
    contains_string_assert(env, html.to_view(), std::string_view("aria-busy=\"true\""))
}

@test
public func components_button_style_passthrough(env : &mut TestEnv) {
    // Arbitrary native attrs (style, data-*) flow through the prop spread.
    var page = HtmlPage()
    #html { <Button style="color:red;" data-test-id="btn-1" title="T">Go</Button> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("style=\"color:red;\""))
    contains_string_assert(env, html.to_view(), std::string_view("data-test-id=\"btn-1\""))
    contains_string_assert(env, html.to_view(), std::string_view("title=\"T\""))
    contains_string_assert(env, html.to_view(), std::string_view(">Go</button>"))
}

// ---------------------------------------------------------------------------
// Badge: variants
// ---------------------------------------------------------------------------

@test
public func components_badge_variants(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <Badge variant="success">Done</Badge> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("data-variant=\"success\""))
    contains_string_assert(env, html.to_view(), std::string_view(">Done</span>"))
    var css = std::string()
    css.append_view(page.getCss())
    contains_string_assert(env, css.to_view(), std::string_view("[data-variant=\"success\"]"))
}

@test
public func components_badge_legacy(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <BadgeAccent>New</BadgeAccent> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("data-variant=\"accent\""))
    contains_string_assert(env, html.to_view(), std::string_view(">New</span>"))
}

// ---------------------------------------------------------------------------
// Alert: variant + title/body
// ---------------------------------------------------------------------------

@test
public func components_alert_variant(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <Alert variant="error">Something broke</Alert> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("role=\"alert\""))
    contains_string_assert(env, html.to_view(), std::string_view("data-variant=\"error\""))
    contains_string_assert(env, html.to_view(), std::string_view(">Something broke</div>"))
    var css = std::string()
    css.append_view(page.getCss())
    contains_string_assert(env, css.to_view(), std::string_view("[data-variant=\"error\"]"))
}

@test
public func components_alert_title_body(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <Alert variant="success"><AlertTitle>Care online</AlertTitle><AlertBody>Same-day visits available.</AlertBody></Alert> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("data-variant=\"success\""))
    contains_string_assert(env, html.to_view(), std::string_view("<h5"))
    contains_string_assert(env, html.to_view(), std::string_view("Care online</h5>"))
    contains_string_assert(env, html.to_view(), std::string_view("<p"))
    contains_string_assert(env, html.to_view(), std::string_view("Same-day visits available.</p>"))
}

// ---------------------------------------------------------------------------
// Card composition
// ---------------------------------------------------------------------------

@test
public func components_card_composition(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <Card><CardHeader><CardTitle>Title</CardTitle><CardDescription>Desc</CardDescription></CardHeader><CardBody>Body</CardBody><CardFooter>Footer</CardFooter></Card> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("Title"))
    contains_string_assert(env, html.to_view(), std::string_view("Desc"))
    contains_string_assert(env, html.to_view(), std::string_view("Body"))
    contains_string_assert(env, html.to_view(), std::string_view("Footer"))
}

// ---------------------------------------------------------------------------
// Typography
// ---------------------------------------------------------------------------

@test
public func components_typography_heading(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <H2>Big Title</H2> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("<h2"))
    contains_string_assert(env, html.to_view(), std::string_view("Big Title</h2>"))
}

@test
public func components_heading_level_dispatch(env : &mut TestEnv) {
    // Heading with level=2 must render <h2> (conditional-return dispatch in SSR).
    var page = HtmlPage()
    #html { <Heading level={2}>Mid Title</Heading> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("<h2"))
    contains_string_assert(env, html.to_view(), std::string_view("Mid Title</h2>"))
}

@test
public func components_heading_level_default(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <Heading>Top Title</Heading> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("<h1"))
    contains_string_assert(env, html.to_view(), std::string_view("Top Title</h1>"))
}

@test
public func components_text_as_dispatch(env : &mut TestEnv) {
    // Text with as="span" must render <span> (conditional-return dispatch).
    var page = HtmlPage()
    #html { <Text as="span" muted={true}>Inline</Text> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("<span"))
    contains_string_assert(env, html.to_view(), std::string_view("data-muted=\"true\""))
    contains_string_assert(env, html.to_view(), std::string_view("Inline</span>"))
}

@test
public func components_link_attrs(env : &mut TestEnv) {
    // Link must render class/href/target/rel/id — previously the object-literal
    // spread ({...attrs}) dropped every attribute in SSR.
    var page = HtmlPage()
    #html { <Link href="/docs" target="_blank" rel="noopener" id="lnk1">Docs</Link> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("<a"))
    contains_string_assert(env, html.to_view(), std::string_view("href=\"/docs\""))
    contains_string_assert(env, html.to_view(), std::string_view("target=\"_blank\""))
    contains_string_assert(env, html.to_view(), std::string_view("rel=\"noopener\""))
    contains_string_assert(env, html.to_view(), std::string_view("id=\"lnk1\""))
    contains_string_assert(env, html.to_view(), std::string_view("Docs</a>"))
}

@test
public func components_typography_text_muted(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <Text muted={true}>Muted text</Text> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("<p"))
    contains_string_assert(env, html.to_view(), std::string_view("data-muted=\"true\""))
    contains_string_assert(env, html.to_view(), std::string_view("Muted text</p>"))
}

@test
public func components_typography_caption(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <Caption>Small caption</Caption> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("<p"))
    contains_string_assert(env, html.to_view(), std::string_view("Small caption</p>"))
}

// ---------------------------------------------------------------------------
// Avatar
// ---------------------------------------------------------------------------

@test
public func components_avatar_fallback(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <Avatar fallback="JT" /> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("JT"))
    contains_string_assert(env, html.to_view(), std::string_view("</span>"))
}

@test
public func components_avatar_src(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <Avatar src="https://example.com/a.png" alt="A" /> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("<img"))
    contains_string_assert(env, html.to_view(), std::string_view("https://example.com/a.png"))
}

// ---------------------------------------------------------------------------
// Toggle
// ---------------------------------------------------------------------------

@test
public func components_toggle_checkbox(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <Checkbox>Accept terms</Checkbox> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("<label"))
    contains_string_assert(env, html.to_view(), std::string_view("type=\"checkbox\""))
    contains_string_assert(env, html.to_view(), std::string_view("Accept terms"))
}

@test
public func components_toggle_switch(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <Switch>Enable</Switch> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("type=\"checkbox\""))
    contains_string_assert(env, html.to_view(), std::string_view("Enable"))
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

@test
public func components_input_renders(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <Input placeholder="Email" /> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("<input"))
    contains_string_assert(env, html.to_view(), std::string_view("placeholder=\"Email\""))
}

@test
public func components_input_variant_filled(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <InputFilled placeholder="Search" /> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("data-variant=\"filled\""))
    contains_string_assert(env, html.to_view(), std::string_view("type=\"text\""))
    var css = std::string()
    css.append_view(page.getCss())
    contains_string_assert(env, css.to_view(), std::string_view("[data-variant=\"filled\"]"))
}

@test
public func components_textarea_renders(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <TextArea placeholder="Message" rows={4}>Body</TextArea> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("<textarea"))
    contains_string_assert(env, html.to_view(), std::string_view("placeholder=\"Message\""))
    contains_string_assert(env, html.to_view(), std::string_view("rows=\"4\""))
    contains_string_assert(env, html.to_view(), std::string_view(">Body</textarea>"))
}

@test
public func components_select_renders_children(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <Select><option>One</option><option>Two</option></Select> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("<select"))
    contains_string_assert(env, html.to_view(), std::string_view("<option>One</option>"))
    contains_string_assert(env, html.to_view(), std::string_view("<option>Two</option>"))
}

@test
public func components_field_renders_label_hint_error(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <Field label="Name" hint="Enter your name" error="Too short"><Input placeholder="Name" /></Field> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("Name"))
    contains_string_assert(env, html.to_view(), std::string_view("Enter your name"))
    contains_string_assert(env, html.to_view(), std::string_view("role=\"alert\""))
    contains_string_assert(env, html.to_view(), std::string_view("Too short"))
}

@test
public func components_field_unlabeled_skips_label(env : &mut TestEnv) {
    // `props.label !== undefined` must evaluate false when the prop is unset
    // (SSR None == "undefined"), so no empty label span is rendered.
    var page = HtmlPage()
    #html { <Field><Input placeholder="Unlabeled" /></Field> }
    var html = std::string()
    html.append_view(page.getHtml())
    var hasEmptyLabel = html.contains(std::string_view("<span class=\"")) && html.contains(std::string_view(">\n        </span>"))
    if(hasEmptyLabel) {
        env.error("unlabeled Field rendered an empty label span")
        env.info(html.data())
    } else {
        env.success("unlabeled Field skips label/hint/error spans")
    }
}

@test
public func components_toggle_radio(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <Radio name="group1" checked={true}>Choice A</Radio> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("type=\"radio\""))
    contains_string_assert(env, html.to_view(), std::string_view("name=\"group1\""))
    contains_string_assert(env, html.to_view(), std::string_view("Choice A"))
}

// ---------------------------------------------------------------------------
// Utilities: Skeleton, Spinner, Divider, Kbd, Stack, Grid, Breadcrumbs
// ---------------------------------------------------------------------------

@test
public func components_utils_skeleton(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <Skeleton width="80px" height="1rem" /> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("<span"))
    contains_string_assert(env, html.to_view(), std::string_view("width:80px;"))
    contains_string_assert(env, html.to_view(), std::string_view("height:1rem;"))
}

@test
public func components_utils_spinner(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <Spinner size="sm" /> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("role=\"status\""))
    contains_string_assert(env, html.to_view(), std::string_view("width:1rem;"))
}

@test
public func components_utils_divider(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <Divider /> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("<hr"))
}

@test
public func components_utils_kbd(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <Kbd>Ctrl+C</Kbd> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("<kbd"))
    contains_string_assert(env, html.to_view(), std::string_view("Ctrl+C</kbd>"))
}

@test
public func components_utils_stack(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <Stack direction="row" gap="sm"><span>A</span><span>B</span></Stack> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("<div"))
    contains_string_assert(env, html.to_view(), std::string_view("A"))
    contains_string_assert(env, html.to_view(), std::string_view("B"))
    var css = std::string()
    css.append_view(page.getCss())
    contains_string_assert(env, css.to_view(), std::string_view("display:flex"))
}

@test
public func components_utils_breadcrumbs(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <Breadcrumbs><BreadcrumbItem><BreadcrumbLink href="/">Home</BreadcrumbLink><BreadcrumbSeparator /></BreadcrumbItem><BreadcrumbItem><BreadcrumbCurrent>Docs</BreadcrumbCurrent></BreadcrumbItem></Breadcrumbs> }
    var html = std::string()
    html.append_view(page.getHtml())
    contains_string_assert(env, html.to_view(), std::string_view("Breadcrumb"))
    contains_string_assert(env, html.to_view(), std::string_view("Home"))
    contains_string_assert(env, html.to_view(), std::string_view("href=\"/\""))
    contains_string_assert(env, html.to_view(), std::string_view("Docs"))
}

// ---------------------------------------------------------------------------
// theme: injectDefaultComponentsTheme registers tokens + keyframes
// ---------------------------------------------------------------------------

@test
public func components_theme_inject(env : &mut TestEnv) {
    var page = HtmlPage()
    page.injectDefaultComponentsTheme()
    var css = std::string()
    css.append_view(page.getCss())
    contains_string_assert(env, css.to_view(), std::string_view("--background"))
    contains_string_assert(env, css.to_view(), std::string_view("--primary"))
    contains_string_assert(env, css.to_view(), std::string_view("--chx-surface"))
    contains_string_assert(env, css.to_view(), std::string_view("chx-spinner-rotate"))
}
