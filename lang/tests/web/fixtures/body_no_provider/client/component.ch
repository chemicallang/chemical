#universal NoProviderProbe(props) {
    var read = () => { console.log(document.titel) }
    return <div onClick={read}>no provider probe</div>
}

func render_fixture(page : &mut HtmlPage) {
    #html { <NoProviderProbe /> }
}
