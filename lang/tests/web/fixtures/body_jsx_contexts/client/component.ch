public struct ContextProps {
    var title : std::string_view
}

#universal ContextProbe(props : ContextProps) {
    var render_body = () => {
        return <div data-title={document.title}>{document.title}</div>
    }
    return render_body()
}

func render_fixture(page : &mut HtmlPage) {
    #html { <ContextProbe title="ok" /> }
}
