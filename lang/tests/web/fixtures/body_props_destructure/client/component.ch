// The universal JS parser does not yet parse object destructuring
// (`var { title } = props`), so the counted-edge fixture uses array
// destructuring: it still reads the props object as a whole, which must not
// produce WEB-BODY-001.
public struct DestructureProps {
    var title : std::string_view
}

#universal DestructureProbe(props : DestructureProps) {
    var [title] = props
    return <div>{title}</div>
}

func render_fixture(page : &mut HtmlPage) {
    #html { <DestructureProbe title="ok" /> }
}
