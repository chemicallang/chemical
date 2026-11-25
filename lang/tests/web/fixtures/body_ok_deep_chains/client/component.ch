// Document -> Element -> ClassList -> method.
#universal ChainClassProbe(props) {
    var read = () => { document.body.classList.contains("x") }
    return <div onClick={read}>class chain</div>
}

// Document -> Option<Element> unwraps -> Element -> method.
#universal ChainOptionProbe(props) {
    var read = () => { document.activeElement.focus() }
    return <div onClick={read}>option chain</div>
}

// Document -> Option<Element> unwraps -> Element -> property method.
#universal ChainQueryProbe(props) {
    var read = () => { document.querySelector("#x").textContent }
    return <div onClick={read}>query chain</div>
}

// Document -> Option<vector<Element>> -> index -> Element -> method.
#universal ChainVectorProbe(props) {
    var read = () => { document.querySelectorAll(".x")[0].focus() }
    return <div onClick={read}>vector chain</div>
}

// Document -> Element -> DOMRect -> field, and Element -> Style -> method.
#universal ChainLayoutProbe(props) {
    var read = () => {
        document.body.getBoundingClientRect().width
        document.body.style.setProperty("color", "red")
    }
    return <div onClick={read}>layout chain</div>
}

func render_fixture(page : &mut HtmlPage) {
    #html {
        <ChainClassProbe />
        <ChainOptionProbe />
        <ChainQueryProbe />
        <ChainVectorProbe />
        <ChainLayoutProbe />
    }
}
