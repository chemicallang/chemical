
struct HtmlElement : HtmlChild {

    var name : std::string_view

    var isSelfClosing : bool

    var attributes : std::vector<*mut HtmlAttribute>

    var children : std::vector<*mut HtmlChild>

    var componentSignature : *mut ComponentSignature = null

    @delete
    func delete(&self) {
        // do nothing
    }

}