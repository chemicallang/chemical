
struct HtmlElement : HtmlChild {

    var name : std::string_view

    var isSelfClosing : bool

    var attributes : std::vector<*HtmlAttribute>

    var children : std::vector<*HtmlChild>

    @delete
    func delete(&self) {
        // do nothing
    }

}