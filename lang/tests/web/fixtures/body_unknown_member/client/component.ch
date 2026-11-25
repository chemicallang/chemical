#universal MemberProbe(props) {
    var read = () => { console.log(document.titel) }
    return <div onClick={read}>member probe</div>
}

func render_fixture(page : &mut HtmlPage) {
    #html { <MemberProbe /> }
}
