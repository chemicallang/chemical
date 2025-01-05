import "./ValueKind.ch"
import "./ASTNode.ch"

struct Value {

    func getKind(&self) : ValueKind

    func link(&self, ptr_ref : **mut Value, resolver : *mut SymbolResolver);

    func getLinkedNode(&self) : *mut ASTNode

}