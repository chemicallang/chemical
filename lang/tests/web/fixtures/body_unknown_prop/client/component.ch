public struct PropProbeProps {
    var title : std::string_view
    var count : int
}

#universal PropProbe(props : PropProbeProps) {
    return <div class="prop-probe">{props.titel}</div>
}

func render_fixture(page : &mut HtmlPage) {
    #html { <PropProbe title="ok" count={1} /> }
}
