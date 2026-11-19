// Embedded-bundle fixture, **phase 1: the build**.
//
// A deployed server should not need the source tree or the output directory: the
// build's bytes belong in the binary. This fixture builds a small site, writes it
// to disk (`finish()`), and then emits the same bytes as Chemical source
// (`write_assets_module`) — the step a two-phase build runs between the site build
// and the program build.
//
// `scripts/web-embedded-test.sh` runs this, then compiles `embedded_consumer`
// against the generated module: that program never builds or reads a site, so a
// byte that only exists because the build wrote a file would be missing there.
using std::string;
using std::string_view;

#universal Badge(props) {
    return <span class="badge">{props.children}</span>
}

// Used by one route only: the unit graph moves its definition into that route's
// chunk, so the embedded set has to carry the chunk too.
#universal Stats(props) {
    return <dl class="stats"><dt>visits</dt><dd>{props.visits}</dd></dl>
}

func render_index(page : &mut HtmlPage) {
    page.appendTitle("Embedded")
    #html {
        <main class="home">
            <h1>Embedded</h1>
            <p>served from the binary</p>
            <Badge>new</Badge>
        </main>
    }
}

func render_stats(page : &mut HtmlPage) {
    page.appendTitle("Stats")
    #html {
        <main class="stats-page">
            <h1>Stats</h1>
            <Stats visits={7} />
        </main>
    }
}

public func main() : int {
    var site = StaticSite.make("lang/compiled/embedded_out", "app", "assets")

    var index = HtmlPage()
    site.add(&mut index, "index.html")
    index.defaultUniversalSetup()
    render_index(&mut index)

    var stats = HtmlPage()
    site.add(&mut stats, "stats/index.html")
    stats.defaultUniversalSetup()
    render_stats(&mut stats)

    if(!site.finish()) {
        unsafe { printf("producer: %s\n", site.error_message().data()) }
        return 1
    }

    // `write_assets_module` creates the module's directory if it is missing.
    var module_path = string_view("lang/compiled/embedded_gen/assets_gen.ch")
    var module_name = string_view("embedded")
    if(!site.write_assets_module(module_path, module_name)) {
        unsafe { printf("producer: could not write the assets module\n") }
        return 1
    }

    unsafe {
        printf("producer: artifacts=%llu chunked=%llu vendor=%llu bundle=%llu B\n",
            site.artifact_count(), site.chunked_unit_count(), site.vendor_unit_count(),
            site.bundle_js_url().size())
    }
    return 0
}
