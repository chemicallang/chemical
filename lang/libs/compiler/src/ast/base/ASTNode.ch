public struct ASTNode {

    func getEncodedLocation(&self) : ubigint

    func getKind(&self) : ASTNodeKind

    func child(&self, name : &std::string_view) : *mut ASTNode

    func declare_top_level(&self, ptr_ref : **mut ASTNode, resolver : *mut SymbolResolver);

    func declare_and_link(&self, ptr_ref : **mut ASTNode, resolver : *mut SymbolResolver);

}
