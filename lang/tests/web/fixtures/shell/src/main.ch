// Phase 0 fixture: one layout, two personalised renders.
//
// `AppShell` is the chrome; the route body arrives as `props.children`. Both
// renders use the same route body and differ only in the two values the chrome
// personalises (`user`, `count`), so the golden pair is exactly what a frozen
// slot shell has to reproduce: chrome bytes identical, only the fill differs.

#universal AppShell(props) {
    return <div class="shell">
        <header class="bar">
            <span class="who">{props.user}</span>
            <span class="count">{props.count}</span>
        </header>
        <main class="content">{props.children}</main>
        <footer class="foot">shell fixture</footer>
    </div>
}

#universal Dashboard(props) {
    return <section class="route dashboard"><h1>Dashboard</h1></section>
}

func render_shell_page(page : &mut HtmlPage, user : std::string_view, count : int) {
    page.appendTitle("Dashboard")
    #html {
        <AppShell user={user} count={count}>
            <Dashboard />
        </AppShell>
    }
}

public func main() : int {
    var site = StaticSite.make("output", "app", "assets")

    var ada = HtmlPage()
    site.add(&mut ada, "ada/index.html")
    ada.defaultUniversalSetup()
    render_shell_page(&mut ada, "ada", 3)

    var grace = HtmlPage()
    site.add(&mut grace, "grace/index.html")
    grace.defaultUniversalSetup()
    render_shell_page(&mut grace, "grace", 5)

    if(!site.finish()) {
        unsafe { printf("shell fixture: %s\n", site.error_message().data()) }
        return 1
    }
    unsafe {
        printf("shell fixture: %llu artifacts chunked_units=%llu retained_units=%llu note=[%s]\n",
            site.artifact_count(), site.chunked_unit_count(), site.retained_unit_count(),
            site.split_note().data())
    }
    return 0
}
