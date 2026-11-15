// Phase 0 fixture: the same component instantiated from one call site three
// times. With source-location hydration ids all three dispatches named the same
// element (so only the first hydrated); with per-instance ids each gets its own.
// Three instances is the minimum that also catches an off-by-one in the counter.

#universal Counter(props) {
    state n = 0
    return <button class="counter" onClick={() => { n = n + 1 }}>{n}</button>
}

#universal Badge(props) {
    return <span class="badge">{props.children}</span>
}

func FixturePage(page : &mut HtmlPage) {
    page.appendTitle("Universal fixture")

    #html {
        <div class="fixture">
            <Counter />
            <Counter />
            <Counter />
            <Badge>one</Badge>
            <Badge>two</Badge>
        </div>
    }
}

public func main() : int {
    var p = HtmlPage()
    FixturePage(&mut p)
    p.writeToDirectory("output", "index", "", "chx-default")
    return 0
}
