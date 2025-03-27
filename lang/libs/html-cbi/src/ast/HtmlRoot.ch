import "./HtmlElement.ch"
import "@compiler/ast/base/ASTNode.ch"

struct HtmlRoot {

    var element : *HtmlElement

    var parent : *mut ASTNode

}