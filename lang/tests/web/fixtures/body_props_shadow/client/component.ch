public struct ShadowProps {
    var title : std::string_view
}

#universal ShadowProbe(props : ShadowProps) {
    var props = "shadow"
    return <div>{props.title}</div>
}

func render_fixture(page : &mut HtmlPage) {
    #html { <ShadowProbe title="ok" /> }
}
