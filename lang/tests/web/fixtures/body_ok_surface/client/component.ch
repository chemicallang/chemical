public struct SurfaceProps {
    var title : std::string_view
}

#universal SurfaceProbe(props : SurfaceProps) {
    useEffect(() => {
        console.log(document.title)
    }, [])
    var on_click = () => {
        var active = document.activeElement
        console.log(Math.round(2))
        window.$__uni_dispatch("noop", null, null)
    }
    return <div class="surface" onClick={on_click}>{props.title}</div>
}

func render_fixture(page : &mut HtmlPage) {
    #html { <SurfaceProbe title="ok" /> }
}
