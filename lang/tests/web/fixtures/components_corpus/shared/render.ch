// Representative render, compiled by both `--web` passes.
public func render_corpus(page : &mut HtmlPage) {
    unsafe {
        #html {
            <div>
                <Button variant="default">Ok</Button>
                <Badge variant="accent">New</Badge>
            </div>
        }
    }
}
