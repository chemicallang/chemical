// Probe 5: a documentation site.
//
// 6 sections x 6 pages, a sectioned sidebar with the active page marked, breadcrumbs,
// prev/next, per-page `<title>`/description/canonical, a sitemap, and zero JavaScript.
//
// What the API makes the author write by hand (gaps in `lang/docs/web-stack-gaps.md`):
//   * the layout: `page` has no layout/partial mechanism, so the chrome is a helper
//     that every page calls — and a page that wants a different sidebar rebuilds it;
//   * navigation data: the site's own structure (sections, order, prev/next) is
//     re-derived in the render code because a build registers pages one by one;
//   * a non-HTML artifact (the search index) has nowhere to go: the export table only
//     holds HTML routes plus sitemap/robots/profile, so a static site cannot ship
//     `search-index.json` and `respond` cannot answer it.
using std::Option;
using std::Result;
using std::string;
using std::string_view;
using std::vector;

// ── Content ───────────────────────────────────────────────────────────────
@direct_init
struct Doc {
    var section : string
    var slug : string
    var title : string
    var summary : string
    var body : string
}

@direct_init
struct Section {
    var slug : string
    var title : string
}

func sections() : vector<Section> {
    var out = vector<Section>()
    out.push_back(Section { slug : "start", title : "Getting started" })
    out.push_back(Section { slug : "language", title : "The language" })
    out.push_back(Section { slug : "web", title : "Web" })
    out.push_back(Section { slug : "stdlib", title : "Standard library" })
    out.push_back(Section { slug : "tooling", title : "Tooling" })
    out.push_back(Section { slug : "deploy", title : "Deploying" })
    return out
}

/// Owned, because the section table is rebuilt per call: a view into it would
/// dangle (the same reason an app cannot keep `Static.add`'s path views).
func section_title(slug : string_view) : string {
    var all = sections()
    var i : ubigint = 0
    while(i < all.size()) {
        var s = unsafe(&*all.get_ptr(i))
        if(s.slug.to_view().equals(&slug)) {
            var out = string()
            out.append_string(&s.title)
            return out
        }
        i = i + 1
    }
    return string()
}

func docs() : vector<Doc> {
    var out = vector<Doc>()
    var all_sections = sections()
    var s : ubigint = 0
    while(s < all_sections.size()) {
        var section = unsafe(&*all_sections.get_ptr(s))
        var section_slug = section.slug.to_view()
        var i : int = 1
        while(i <= 6) {
            var slug = string("page-")
            slug.append_integer(i as bigint)
            var section_name = section_title(section_slug)
            var section_name_view = section_name.to_view()
            var title = string()
            title.append_integer(i as bigint)
            title.append_view(". ")
            title.append_view(&section_name_view)
            var summary = string("Page ")
            summary.append_integer(i as bigint)
            summary.append_view(" in the ")
            summary.append_view(&section_name_view)
            summary.append_view(" chapter.")
            var body = string("## Overview\n\n")
            body.append_view("This is page ")
            body.append_integer(i as bigint)
            body.append_view(" of the ")
            body.append_view(&section_name_view)
            body.append_view(" chapter. A real page would come from a file")
            body.append_view(" or a CMS; here it is generated so the measurement is")
            body.append_view(" about the build, not the words.\n\n")
            body.append_view("### A code sample\n\n```chemical\nfunc add(a : int, b : int) : int {\n    return a + b\n}\n```\n\n")
            body.append_view("- one point\n- two points\n- **three** points\n\n")
            body.append_view("> A note, so the renderer produces nested blocks.\n")
            out.push_back(Doc {
                section : string(section_slug),
                slug : slug,
                title : title,
                summary : summary,
                body : body
            })
            i = i + 1
        }
        s = s + 1
    }
    return out
}

func no_highlight(lang : string_view, code : string_view) : string {
    return string()
}

func no_rewrite(url : string_view) : string {
    return unsafe(string(url.data(), url.size()))
}

func doc_html(doc : &Doc) : string {
    var view = doc.body.to_view()
    return md::to_html(view, no_highlight, no_rewrite)
}

// A flat index is what the prev/next links and the search index need; the site has
// no notion of "all pages", so the app carries it.
func find_doc(all : &vector<Doc>, section : string_view, slug : string_view) : int {
    var i : ubigint = 0
    while(i < all.size()) {
        var d = unsafe(&*all.get_ptr(i))
        if(d.section.to_view().equals(&section) && d.slug.to_view().equals(&slug)) {
            return i as int
        }
        i = i + 1
    }
    return -1
}

func doc_path(doc : &Doc) : string {
    var out = string()
    out.append_string(&doc.section)
    out.append('/')
    out.append_string(&doc.slug)
    out.append_view("/")
    return out
}

// The whole chrome. Called by every page because there is no layout mechanism.
func layout(page : &mut HtmlPage, all : &vector<Doc>, doc : &Doc, index : int) {
    page.appendTitle(doc.title.to_view())
    var canonical = doc_path(doc)
    var canonical_view = canonical.to_view()
    var head = Head {
        description : doc.summary.to_view(),
        canonical : canonical_view
    }
    page.set_head(&head)
    var body = doc_html(doc)
    var body_view = body.to_view()
    // Prev/next markup is prepared as text: `#html` has no conditional markup and
    // a bare `index` inside an escape does not resolve to the parameter.
    var section_name = section_title(doc.section.to_view())
    var section_name_view = section_name.to_view()
    var section_href = section_href_of(doc)
    var section_href_view = section_href.to_view()
    var prev_markup = pager_link(all, index - 1, true)
    var next_markup = pager_link(all, page_next(all, index), false)
    var prev_view = prev_markup.to_view()
    var next_view = next_markup.to_view()
    var all_sections = sections()
    var si : ubigint = 0
    #html {
        <div class="shell">
            <aside class="sidebar">
                <a class="brand" href="/">Chemical</a>
                <nav>
                    @{
                        while(si < all_sections.size()) {
                            var section = unsafe(&*all_sections.get_ptr(si))
                            var section_slug = section.slug.to_view()
                            var nav_href = string("/")
                            nav_href.append_view(&section_slug)
                            nav_href.append_view("/")
                            var nav_href_view = nav_href.to_view()
                            var section_heading = section.title.to_view()
                            #html {
                                <h2 class="section">{section_heading}</h2>
                            }
                            var di : ubigint = 0
                            while(di < all.size()) {
                                var item = unsafe(&*all.get_ptr(di))
                                if(item.section.to_view().equals(&section_slug)) {
                                    var is_active = di == (index as ubigint)
                                    var href = string("/")
                                    href.append_string(&item.section)
                                    href.append('/')
                                    href.append_string(&item.slug)
                                    href.append_view("/")
                                    var href_view = href.to_view()
                                    var link_class = nav_class(is_active)
                                    var item_title = item.title.to_view()
                                    #html {
                                        <a class={link_class} href={href_view}>{item_title}</a>
                                    }
                                }
                                di = di + 1
                            }
                            si = si + 1
                        }
                    }
                </nav>
            </aside>
            <main class="doc">
                <nav class="crumbs">
                    <a href="/">Docs</a> / <a href={section_href_view}>{section_name_view}</a>
                </nav>
                <h1>{doc.title.to_view()}</h1>
                <div class="body">{body_view}</div>
                <nav class="pager">{prev_view}{next_view}</nav>
            </main>
        </div>
    }
}

func nav_class(active : bool) : string_view {
    if(active) { return string_view("nav-link active") }
    return string_view("nav-link")
}

func section_href_of(doc : &Doc) : string {
    var out = string("/")
    out.append_string(&doc.section)
    out.append('/')
    return out
}

/// The prev/next link as plain text, or `""` when there is none.
func pager_link(all : &vector<Doc>, index : int, previous : bool) : string {
    var out = string()
    if(index < 0 || index >= (all.size() as int)) { return out }
    var target = unsafe(&*all.get_ptr(index as ubigint))
    var href = string("/")
    href.append_string(&target.section)
    href.append('/')
    href.append_string(&target.slug)
    href.append_view("/")
    if(previous) { out.append_view("<a class=\"prev\" href=\"") }
    else { out.append_view("<a class=\"next\" href=\"") }
    out.append_string(&href)
    out.append_view("\">")
    if(previous) { out.append_view("Previous: ") } else { out.append_view("Next: ") }
    out.append_string(&target.title)
    out.append_view("</a>")
    return out
}

func page_prev(all : &vector<Doc>, index : int) : int {
    return index - 1
}

func page_next(all : &vector<Doc>, index : int) : int {
    var limit = all.size() as int
    if(index + 1 < limit) { return index + 1 }
    return -1
}

// ── Section index and home ────────────────────────────────────────────────
func render_section(page : &mut HtmlPage, all : &vector<Doc>, section : &Section) {
    var title = section.title.to_view()
    page.appendTitle(&title)
    var slug = section.slug.to_view()
    var di : ubigint = 0
    #html {
        <div class="shell">
            <aside class="sidebar"><a class="brand" href="/">Chemical</a></aside>
            <main class="doc">
                <h1>{title}</h1>
                <ul class="section-list">
                    @{
                        while(di < all.size()) {
                            var item = unsafe(&*all.get_ptr(di))
                            if(item.section.to_view().equals(&slug)) {
                                var href = string("/")
                                href.append_string(&item.section)
                                href.append('/')
                                href.append_string(&item.slug)
                                href.append_view("/")
                                var href_view = href.to_view()
                                var item_title = item.title.to_view()
                                var item_summary = item.summary.to_view()
                                #html {
                                    <li><a href={href_view}>{item_title}</a><p>{item_summary}</p></li>
                                }
                            }
                            di = di + 1
                        }
                    }
                </ul>
            </main>
        </div>
    }
}

func render_home(page : &mut HtmlPage, all_sections : &vector<Section>) {
    page.appendTitle("Chemical documentation")
    var si : ubigint = 0
    #html {
        <div class="shell">
            <aside class="sidebar"><a class="brand" href="/">Chemical</a></aside>
            <main class="doc">
                <h1>Chemical documentation</h1>
                <p>Thirty-six pages across six chapters.</p>
                <ul class="sections">
                    @{
                        while(si < all_sections.size()) {
                            var section = unsafe(&*all_sections.get_ptr(si))
                            var href = string("/")
                            href.append_string(&section.slug)
                            href.append_view("/")
                            var href_view = href.to_view()
                            var section_heading = section.title.to_view()
                            #html {
                                <li><a href={href_view}>{section_heading}</a></li>
                            }
                            si = si + 1
                        }
                    }
                </ul>
            </main>
        </div>
    }
}

// The search index the docs front-end fetches: one entry per page. Before
// `Static.add_artifact` existed this had nowhere to go — the artifact set was HTML
// routes plus sitemap/robots/profile.
func search_index(all : &vector<Doc>) : string {
    var out = string("[")
    var i : ubigint = 0
    while(i < all.size()) {
        var doc = unsafe(&*all.get_ptr(i))
        if(i > 0u) { out.append(',') }
        out.append_view("{\"section\":\"")
        out.append_string(&doc.section)
        out.append_view("\",\"slug\":\"")
        out.append_string(&doc.slug)
        out.append_view("\",\"title\":\"")
        out.append_string(&doc.title)
        out.append_view("\",\"summary\":\"")
        out.append_string(&doc.summary)
        out.append_view("\"}")
        i = i + 1
    }
    out.append(']')
    return out
}

// ── Build ─────────────────────────────────────────────────────────────────
func string_of(v : string_view) : string {
    var out = string()
    out.append_view(&v)
    return out
}

func check(name : string_view, ok : bool) {
    var n = name
    if(ok) {
        unsafe { printf("ok   %.*s\n", n.size() as int, n.data()) }
    } else {
        unsafe { printf("FAIL %.*s\n", n.size() as int, n.data()) }
    }
}

public func main() : int {
    var started = std::chrono::Instant::now()
    var all = docs()
    var all_sections = sections()

    var site = web::Static.make("lang/compiled/docs_probe", "docs", "assets")
    site.set_minify(true)
    site.set_gzip(true)
    site.export_sitemap("https://docs.example", true)

    var index_json = search_index(&all)
    var index_view = index_json.to_view()
    var json_type = string_view("application/json; charset=utf-8")
    site.add_artifact("search-index.json", index_view, json_type)

    // The site owns every page (`Static.page`), so the natural loop is safe: the
    // previous API (`add` + a caller-owned page) read freed memory here and wrote
    // garbage HTML, silently.
    var home = site.page("index.html")
    render_home(home, &all_sections)

    var si : ubigint = 0
    while(si < all_sections.size()) {
        var section = unsafe(&*all_sections.get_ptr(si))
        var path = string()
        path.append_string(&section.slug)
        path.append_view("/index.html")
        var path_view = path.to_view()
        var page = site.page(path_view)
        render_section(page, &all, section)
        si = si + 1
    }

    var di : ubigint = 0
    while(di < all.size()) {
        var doc = unsafe(&*all.get_ptr(di))
        var path = string()
        path.append_string(&doc.section)
        path.append('/')
        path.append_string(&doc.slug)
        path.append_view("/index.html")
        var path_view = path.to_view()
        var page = site.page(path_view)
        layout(page, &all, doc, di as int)
        di = di + 1
    }

    var built = site.build()
    var msg = site.error_message()
    unsafe { printf("build=%d msg=%.*s\n", built as int, msg.size() as int, msg.data()) }
    if(!built) { return 1 }

    var elapsed = started.elapsed()
    var build_us = elapsed.as_micros()
    var artifacts = site.artifact_count()
    var pages = site.page_count()
    var total_bytes : ubigint = 0
    var html_pages : ubigint = 0
    var html_bytes : ubigint = 0
    var biggest : ubigint = 0
    var scripts : ubigint = 0
    var i : ubigint = 0
    while(i < artifacts) {
        var path = site.artifact_path(i)
        var bytes = site.artifact_bytes(i)
        total_bytes = total_bytes + bytes.size()
        var is_html = path.size() >= 5u
        if(is_html) {
            var tail = path.subview(path.size() - 5u, path.size())
            is_html = tail.equals(&string_view(".html"))
        }
        if(is_html) {
            html_pages = html_pages + 1
            html_bytes = html_bytes + bytes.size()
            if(bytes.size() > biggest) { biggest = bytes.size() }
            if(bytes.find(&string_view("<script")) is Option.Some) { scripts = scripts + 1u }
        }
        i = i + 1
    }
    unsafe {
        printf("measured pages=%zu build_us=%lld artifacts=%zu total_bytes=%zu html_pages=%zu html_bytes=%zu biggest=%zu scripts=%zu\n",
            pages, build_us, artifacts, total_bytes, html_pages, html_bytes, biggest, scripts)
    }

    // Client bytes: a site with no component and no client module must ship none.
    var bundle = string_of(site.bundle_js_url())
    var bundle_view = bundle.to_view()
    unsafe { printf("bundle=%.*s gzip=%d\n", bundle_view.size() as int, bundle_view.data(), site.has_gzip_variants() as int) }
    check(string_view("a component-free docs site emits no JS bundle"), bundle.size() == 0)
    check(string_view("no page carries a <script> tag"), scripts == 0u)
    check(string_view("gzip variants exist for the HTML"), site.has_gzip_variants())

    // Serving from RAM.
    // `respond()` returns the negotiated body; with gzip on and no `Accept-Encoding`
    // argument it hands back the compressed variant, so ask for identity explicitly.
    var doc_page = site.respond_negotiated(string_view("/web/page-3/"),
        string_view(""), string_view(""))
    var doc_body = doc_page.body
    unsafe { printf("DEBUG status=%d enc=%.*s bytes=%zu head=%.*s\n", doc_page.status, doc_page.content_encoding.size() as int, doc_page.content_encoding.data(), doc_body.size(), 200, doc_body.data()) }
    check(string_view("a doc route is served from the artifact table"), doc_page.status == 200u)
    check(string_view("the doc route keeps its title"), doc_body.contains(&string_view("3. Web")))
    check(string_view("the sidebar marks the active page"),
        doc_body.contains(&string_view("nav-link active")))
    check(string_view("the page has prev/next links"),
        doc_body.contains(&string_view("Previous:")) && doc_body.contains(&string_view("Next:")))
    check(string_view("the sitemap is exported"),
        site.respond(string_view("/sitemap.xml")).status == 200u)

    // A non-HTML artifact is part of the set: served, content-typed, and written.
    var search = site.respond_negotiated(string_view("/search-index.json"),
        string_view(""), string_view(""))
    check(string_view("the search index is served"), search.status == 200)
    check(string_view("the search index is JSON"),
        search.content_type.equals(&string_view("application/json; charset=utf-8")))
    check(string_view("the search index carries every page"),
        search.body.contains(&string_view("\"slug\":\"page-6\"")))

    var finished = site.finish()
    unsafe { printf("finish=%d\n", finished as int) }
    return 0
}
