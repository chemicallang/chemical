public struct ASTNode {

    func getEncodedLocation(&self) : ubigint

    func getKind(&self) : ASTNodeKind

    // may return null
    func getParent(&self) : *mut ASTNode

    func child(&self, name : &std::string_view) : *mut ASTNode

    func getAccessSpecifier(&self) : AccessSpecifier

}