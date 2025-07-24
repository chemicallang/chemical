public struct ASTNode {

    func getEncodedLocation(&self) : ubigint

    func getKind(&self) : ASTNodeKind

    func child(&self, name : &std::string_view) : *mut ASTNode

}
