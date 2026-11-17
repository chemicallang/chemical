// Phase 0 fixture: pure `#html`, two pages, nothing that can emit JavaScript.
//
// The invariant under test (§3.20, "a static page still ships no JS") is that a
// page using none of the web-stack features writes no `.js` artifact and no
// inline `<script>`. `scripts/web-golden-test.sh` asserts both on the output
// tree, so a future change that starts shipping a runtime everywhere fails
// here rather than in a browser.
//
// Both names go into one flat directory because `writeToDirectory` creates only
// the directory it is given; nested paths are what `StaticSite` is for.

func render_home(page : &mut HtmlPage) {
    page.appendTitle("Static home")
    #html {
        <main class="home">
            <h1>Static home</h1>
            <p>This page has no JavaScript at all.</p>
            <a href="about.html">About</a>
        </main>
    }
}

func render_about(page : &mut HtmlPage) {
    page.appendTitle("Static about")
    #html {
        <main class="about">
            <h1>About</h1>
            <p>Still no JavaScript.</p>
            <a href="index.html">Home</a>
        </main>
    }
}

public func main() : int {
    var home = HtmlPage()
    render_home(&mut home)
    home.writeToDirectory("output", "index")

    var about = HtmlPage()
    render_about(&mut about)
    about.writeToDirectory("output", "about")
    return 0
}
