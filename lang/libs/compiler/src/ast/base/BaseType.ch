import "./BaseTypeKind.ch"
import "../../SymbolResolver.ch"

public struct BaseType {

    func getKind(&self) : BaseTypeKind

    func link(&self, ptr_ref : **mut BaseType, resolver : *mut SymbolResolver) : bool

    func getLinkedNode(&self) : *mut ASTNode

}