// Phase 2 fixture: typed `#universal` props.
// `Card(props : CardProps)` declares its prop surface; the call sites below are
// the compile-time contract. `BadCall` is only rendered when TYPED_PROPS_NEGATIVE
// is defined, so the same file proves both directions:
//
//   chemical ... --define TYPED_PROPS_NEGATIVE   -> must FAIL with
//       "unknown prop 'counts' for component 'Card'; declared props: title, count"
public struct CardProps {
    var title : std::string_view
    var count : int
}

#universal Card(props : CardProps) {
    return <div class="card">{props.title}</div>
}

// Implicit props (children, class, on*) are always allowed even though they are
// not declared in CardProps.
#universal Wrapper(props) {
    return <section class="wrap">{props.children}</section>
}

func TypedPropsPage(page : &mut HtmlPage) {
    page.appendTitle("Typed props fixture")

    #html {
        <Wrapper>
            <Card title="first" count={1} />
            <Card title="second" count={2} className="extra" />
        </Wrapper>
    }
}

public func main() : int {
    var p = HtmlPage()
    TypedPropsPage(&mut p)
    p.writeToDirectory("output", "index", "", "chx-default")
    return 0
}
