import "./ValueKind.ch"
import "./ASTNode.ch"
import "./BaseType.ch"
import "../../SymbolResolver.ch"

struct Value {

    func getKind(&self) : ValueKind

    func link(&self, ptr_ref : **mut Value, type : *mut BaseType, resolver : *mut SymbolResolver) : bool

    func getLinkedNode(&self) : *mut ASTNode

}