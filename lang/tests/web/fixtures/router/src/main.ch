// Phase 0 fixture: route selection, per-route titles, and the noindex on a miss.
//
// Selection is by router id (`add_parameter_view`) rather than by URL. URL
// deep-link selection is covered by `lang/tests/libs/router/src/url.ch`; what a
// golden can pin — and those tests cannot — is the *served artifact*: the
// selected route's `<title>` in the head, an unselected route's title absent,
// the route's wrapper marked active, and `<meta name="robots" content="noindex">`
// on a page marked route-missing.
//
// The page also renders a `<NavLink>`, so importing the whole `router` library is
// part of the gate: `RouterLink`/`NavLink` used to read `window` during render,
// which the hydration-safety analysis rejects, and that made the library
// unimportable. Their URL active state is now applied in an effect, so SSR
// renders the link without `aria-current` and hydration adds it — no mismatch.

#universal RouterApp(props) {
    router "main" {
        route default #"home" title "Home" {
            <div class="view home"><h1>Home</h1></div>
        }
        route #"about" title "About" {
            <div class="view about"><h1>About</h1></div>
        }
        route #"project" title "Project" {
            <div class="view project"><h1>Project</h1></div>
        }
    }
}

func render_home(page : &mut HtmlPage) {
    page.appendTitle("Fixture")
    #html {
        <nav class="nav"><NavLink href="/about">About</NavLink></nav>
        <RouterApp />
    }
}

func render_about(page : &mut HtmlPage) {
    page.appendTitle("Fixture")
    page.add_parameter_view("main", "about")
    #html { <RouterApp /> }
}

// A page that matched nothing: the default route still renders, but the head
// must not let a crawler index it as the real page.
func render_missing(page : &mut HtmlPage) {
    page.appendTitle("Fixture")
    page.mark_route_missing()
    #html { <RouterApp /> }
}

public func main() : int {
    var site = StaticSite.make("output", "app", "assets")

    var home = HtmlPage()
    site.add(&mut home, "home/index.html")
    home.defaultUniversalSetup()
    render_home(&mut home)

    var about = HtmlPage()
    site.add(&mut about, "about/index.html")
    about.defaultUniversalSetup()
    render_about(&mut about)

    var missing = HtmlPage()
    site.add(&mut missing, "missing/index.html")
    missing.defaultUniversalSetup()
    render_missing(&mut missing)

    if(!site.finish()) {
        unsafe { printf("router fixture: %s\n", site.error_message().data()) }
        return 1
    }
    unsafe { printf("router fixture: %llu artifacts\n", site.artifact_count()) }
    return 0
}
