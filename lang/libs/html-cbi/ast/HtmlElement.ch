import "@std/string_view.ch"
import "@std/vector.ch"
import "@compiler/ASTBuilder.ch"
import "./HtmlAttribute.ch"
import "./HtmlChild.ch"

struct HtmlElement : HtmlChild {

    var name : std::string_view

    var attributes : std::vector<*HtmlAttribute>

    var children : std::vector<*HtmlChild>

    @delete
    func delete(&self) {
        // do nothing
    }

}