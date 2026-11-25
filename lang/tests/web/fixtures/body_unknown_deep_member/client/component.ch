// Phase 5b negative: the chain root is `document`, and every step is typed
// (`Document` -> `Element`), so a missing member on the final step is an error
// exactly like a direct ambient-root member.
#universal DeepMemberProbe(props) {
    var read = () => { console.log(document.body.textContnt) }
    return <div onClick={read}>deep member probe</div>
}

func render_fixture(page : &mut HtmlPage) {
    #html { <DeepMemberProbe /> }
}
