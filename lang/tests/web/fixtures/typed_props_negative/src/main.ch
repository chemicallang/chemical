// Negative fixture: a prop typo must be a compile error, not `undefined` in the
// browser. Expected diagnostic:
//   unknown prop 'counts' for component 'Card'; declared props: title, count
public struct CardProps {
    var title : std::string_view
    var count : int
}

#universal Card(props : CardProps) {
    return <div class="card">{props.title}</div>
}

func BadPage(page : &mut HtmlPage) {
    #html {
        <Card title="ok" counts={1} />
    }
}

public func main() : int {
    var p = HtmlPage()
    BadPage(&mut p)
    p.writeToDirectory("output", "index", "", "chx-default")
    return 0
}
