public struct ChainProps {
    var title : std::string_view
}

#universal ChainProbe(props : ChainProps) {
    var on_click = () => { console.log(document.body.classList) }
    return <div onClick={on_click}>{props.title}{props.children}</div>
}

func render_fixture(page : &mut HtmlPage) {
    #html { <ChainProbe title="ok" /> }
}
