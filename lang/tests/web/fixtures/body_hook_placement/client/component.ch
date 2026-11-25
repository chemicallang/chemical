#universal HookProbe(props) {
    state enabled = false
    if(enabled) {
        useEffect(() => { console.log("mounted") }, [])
    }
    return <div>hook probe</div>
}

func render_fixture(page : &mut HtmlPage) {
    #html { <HookProbe /> }
}
