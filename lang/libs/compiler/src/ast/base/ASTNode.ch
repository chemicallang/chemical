public struct ASTNode {

    func getEncodedLocation(&self) : ubigint

    func getKind(&self) : ASTNodeKind

    func child(&self, name : &std::string_view) : *mut ASTNode

    func declare_and_link(&self, ptr_ref : **mut ASTNode, resolver : *mut SymbolResolver);

}
