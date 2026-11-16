// Baseline for the static-site comparison: the same two pages as
// `fixtures/static_site`, rendered with plain `HtmlPage.toString()`.
//
// Every page therefore carries its own copy of the hydration runtime and both
// component definitions — the shape the plan measured at 78.7% duplication. The
// probe compares this output with `static_site`'s to quantify what sharing buys.

#universal Counter(props) {
    state n = 0
    return <button class="counter" onClick={() => { n = n + 1 }}>{n}</button>
}

#universal Badge(props) {
    return <span class="badge">{props.children}</span>
}

func render_index(page : &mut HtmlPage) {
    page.appendTitle("Index")
    #html {
        <div class="page index">
            <h1>Index</h1>
            <Counter />
            <Badge>shared</Badge>
        </div>
    }
}

func render_about(page : &mut HtmlPage) {
    page.appendTitle("About")
    #html {
        <div class="page about">
            <h1>About</h1>
            <Counter />
            <Badge>shared</Badge>
        </div>
    }
}

func write_page(path : std::string_view, html : std::string) {
    unsafe {
        fs::raw::write_text_file(path.data(), html.data() as *u8, html.size())
    }
}

public func main() : int {
    unsafe {
        fs::raw::mkdir("lang/compiled/static_site_baseline_out")
        fs::raw::mkdir("lang/compiled/static_site_baseline_out/about")
    }

    var index = HtmlPage()
    index.defaultUniversalSetup()
    render_index(&mut index)
    write_page("lang/compiled/static_site_baseline_out/index.html", index.toString())

    var about = HtmlPage()
    about.defaultUniversalSetup()
    render_about(&mut about)
    write_page("lang/compiled/static_site_baseline_out/about/index.html", about.toString())

    unsafe { printf("baseline written\n") }
    return 0
}
