import "./CSSDeclaration.ch"
import "@std/vector.ch"
import "@compiler/ast/base/ASTNode.ch"

struct CSSOM {

    var parent : *mut ASTNode

    var declarations : std::vector<*mut CSSDeclaration>

}