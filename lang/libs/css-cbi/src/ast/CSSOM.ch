import "./CSSDeclaration.ch"
import "@std/vector.ch"
import "@compiler/ast/base/ASTNode.ch"

struct CSSOM {

    var parent : *mut ASTNode

    // if NOT has dynamic values, we will automatically put
    // class name that is hashed and prefixed with 'h'
    var has_dynamic_values : bool

    var declarations : std::vector<*mut CSSDeclaration>

}