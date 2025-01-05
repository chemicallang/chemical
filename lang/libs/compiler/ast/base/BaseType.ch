import "./BaseTypeKind.ch"
import "../../SymbolResolver.ch"

struct BaseType {

    func getKind(&self) : BaseTypeKind

    func link(&self, ptr_ref : **mut BaseType, resolver : *mut SymbolResolver);

    func getLinkedNode(&self) : *mut ASTNode

}