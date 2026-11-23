using namespace std;

// ---------------------------------------------------------------------------
// Page-level minification: `HtmlPage.set_minify` / `StaticSite.set_minify`.
//
// The universal runtime and every component definition are machine-generated JS:
// four-space indentation and explanatory comments, which are the largest single
// block of removable bytes in a page bundle. None of it may be removed from the
// library — the emitted source is what a component author reads — so the switch
// lives on the artifact: serialization writes minified bytes while the render
// buffers stay as they were.
//
// These tests pin three things: off changes nothing, on shrinks both JS and CSS,
// and a `StaticSite` build minifies the *route chunks* too (not only the shared
// bundle), which is the half that used to be forgotten.
// ---------------------------------------------------------------------------

func pm_has(hay : &string_view, needle : &string_view) : bool {
    return hay.find_or_npos(needle) != NPOS
}

func pm_fail(env : &mut TestEnv, what : string_view) {
    unsafe(test_env::ffi::log_cstr(env, LogType.Error, what.data(), 0, 0))
}

// A page carrying the hydration runtime, one hand-written dispatch statement and
// a stylesheet, which is the shape every universal page has.
func pm_page() : HtmlPage {
    var p = HtmlPage()
    p.defaultPrepare()
    p.defaultUniversalSetup()
    p.append_css("/* theme */\n.chx-card {\n    color: red;\n}\n")
    // A comment in the page's own tail marks its route chunk: the shared bundle
    // never carries it, so finding it in an artifact means the chunk was written
    // unminified.
    p.append_js_end("window.$__uni_c('a', document.body)\n// page chunk comment\n")
    return p
}

func pm_site_dir() : string {
    var dir = unsafe(string(intrinsics::get_build_dir()))
    dir.append_view(string_view("/site_minify_test"))
    unsafe(fs::raw::mkdir(dir.data()))
    return dir
}

@test
func test_page_minify_default_keeps_source(env : &mut TestEnv) {
    var p = pm_page()
    if(p.is_minify_enabled()) {
        pm_fail(env, string_view("minification must be off by default"))
    }
    var html = p.toString()
    var view = html.to_view()
    // Indentation and comments of the emitted runtime are the development view.
    if(!pm_has(&view, string_view("\n    "))) {
        pm_fail(env, string_view("unminified output must keep indentation"))
    }
    if(!pm_has(&view, string_view("/* theme */"))) {
        pm_fail(env, string_view("unminified output must keep CSS comments"))
    }
}

@test
func test_page_minify_shrinks_js_and_css(env : &mut TestEnv) {
    var raw_page = pm_page()
    var raw = raw_page.toString()
    var raw_view = raw.to_view()

    var p = pm_page()
    p.set_minify(true)
    if(!p.is_minify_enabled()) {
        pm_fail(env, string_view("set_minify(true) must be observable"))
    }
    var html = p.toString()
    var view = html.to_view()

    // The render buffers are not rewritten: only what is serialized shrinks.
    var own_js = p.toStringJsOnly()
    var own_js_view = own_js.to_view()
    if(own_js.size() == 0 || !pm_has(&own_js_view, string_view("\n"))) {
        pm_fail(env, string_view("the page's JS buffer must stay as it was rendered"))
    }
    if(view.size() >= raw_view.size()) {
        pm_fail(env, string_view("minified page must be smaller than the unminified one"))
        return
    }
    if(pm_has(&view, string_view("/* theme */"))) {
        pm_fail(env, string_view("minified page must drop CSS comments"))
    }
    // Minification removes bytes, never code: the runtime entry point and the
    // dispatch statement have to survive.
    if(!pm_has(&view, string_view("window.$__uni_c"))) {
        pm_fail(env, string_view("minified page must still define the runtime"))
    }
    if(!pm_has(&view, string_view("window.$__uni_c('a',document.body)"))) {
        pm_fail(env, string_view("minified dispatch statement must keep its tokens"))
    }
}

@test
func test_page_minify_writes_minified_assets(env : &mut TestEnv) {
    var dir = pm_site_dir()
    var p = pm_page()
    p.set_minify(true)
    p.writeToDirectory(dir.to_view(), string_view("index"))

    var js_path = dir.copy()
    js_path.append_view(string_view("/index.js"))
    var js_res = unsafe(fs::raw::read_entire_file(js_path.data()))
    if(js_res is Result.Err) {
        pm_fail(env, string_view("writeToDirectory must write index.js"))
        return
    }
    var Ok(js_bytes) = js_res else unsafe { unreachable }
    var js = unsafe(string(js_bytes.data() as *char, js_bytes.size()))
    var js_view = js.to_view()

    var raw_page = pm_page()
    var raw_html = raw_page.toString()
    var raw_view = raw_html.to_view()
    if(js.size() == 0 || js.size() >= raw_view.size()) {
        pm_fail(env, string_view("index.js must be the minified bundle"))
        return
    }
    if(pm_has(&js_view, string_view("\n    "))) {
        pm_fail(env, string_view("index.js must not carry the emitter's indentation"))
    }

    var css_path = dir.copy()
    css_path.append_view(string_view("/index.css"))
    var css_res = unsafe(fs::raw::read_entire_file(css_path.data()))
    if(css_res is Result.Err) {
        pm_fail(env, string_view("writeToDirectory must write index.css"))
        return
    }
    var Ok(css_bytes) = css_res else unsafe { unreachable }
    var css = unsafe(string(css_bytes.data() as *char, css_bytes.size()))
    var css_view = css.to_view()
    if(pm_has(&css_view, string_view("/* theme */")) || pm_has(&css_view, string_view("\n"))) {
        pm_fail(env, string_view("index.css must be minified"))
    }
}

// The switch is on the site, but half the bytes the build publishes are per-page
// chunks. Without propagating it, `set_minify(true)` shrank the shared bundle and
// left every route chunk — the route's own component definitions — readable.
@test
func test_site_minify_minifies_route_chunks(env : &mut TestEnv) {
    var dir_s = pm_site_dir()
    var dir = dir_s.to_view()

    var site = StaticSite.make(dir)
    site.set_minify(true)

    // Render *after* attaching: while attached the runtime goes to the shared
    // bundle, which is what the build freezes.
    var p = HtmlPage()
    p.defaultPrepare()
    site.add(&mut p, string_view("index.html"))
    p.defaultUniversalSetup()
    p.append_css("/* theme */\n.chx-card { color: red; }\n")
    // A comment in the page's own tail marks its route chunk: the shared bundle
    // never carries it, so finding it in an artifact means the chunk was written
    // unminified.
    p.append_js_end("window.$__uni_c('a', document.body)\n// page chunk comment\n")
    if(!site.build()) {
        pm_fail(env, string_view("site build with minification failed"))
        var why = site.error_message()
        if(why.size() > 0) {
            unsafe(test_env::ffi::log_cstr(env, LogType.Information, why.data(), 0, 0))
        }
        return
    }

    var chunks : ubigint = 0
    var i : ubigint = 0
    while(i < site.artifact_count()) {
        var path = site.artifact_path(i)
        var is_js = false
        if(path.size() > 3) {
            var tail = path.subview(path.size() - 3, path.size())
            is_js = tail.equals(&string_view(".js"))
        }
        if(is_js) {
            var bytes = site.artifact_bytes(i)
            chunks = chunks + 1
            // A minified chunk has no indentation, and the runtime it links is
            // written without spaces around `=`.
            if(pm_has(&bytes, string_view("// page chunk comment"))) {
                pm_fail(env, string_view("a site-minified route chunk kept its comments"))
            }
        }
        i = i + 1
    }
    if(chunks == 0) {
        pm_fail(env, string_view("a site with the universal runtime must publish JS artifacts"))
    }
}
