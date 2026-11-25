public struct ChainProps {
    var title : std::string_view
}

// Phase 5b: chains rooted at an ambient provider follow one or more members.
// Every member below exists on the linked `dom` surface (Element, ClassList,
// Style, DOMRect, Option<Element>) so none is a WEB-BODY-002. The local/unknown
// bases still count WEB-BODY-005 edges; they are never errors.
#universal ChainProbe(props : ChainProps) {
    var on_click = () => {
        document.body.classList.contains("x")
        document.activeElement.focus()
        document.querySelector("#x").textContent
        document.querySelectorAll(".x")
        document.body.getBoundingClientRect().width
        document.body.style.setProperty("color", "red")
        var unknown_local = some_function()
        unknown_local.titel
        console.log(document.body.classList)
    }
    return <div onClick={on_click}>{props.title}{props.children}</div>
}

func render_fixture(page : &mut HtmlPage) {
    #html { <ChainProbe title="ok" /> }
}
