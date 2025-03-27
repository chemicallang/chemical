import "ASTNodeKind.ch"
import "../../SymbolResolver.ch"

public struct ASTNode {

    func getKind(&self) : ASTNodeKind

    func declare_top_level(&self, ptr_ref : **mut ASTNode, resolver : *mut SymbolResolver);

    func declare_and_link(&self, ptr_ref : **mut ASTNode, resolver : *mut SymbolResolver);

}
