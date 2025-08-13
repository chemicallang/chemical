public struct Value : ASTAny {

    func getEncodedLocation(&self) : ubigint

    func getKind(&self) : ValueKind

    func getLinkedNode(&self) : *mut ASTNode

}