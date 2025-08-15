public struct Value : ASTAny {

    func getEncodedLocation(&self) : ubigint

    func getKind(&self) : ValueKind

    func getType(&self) : *mut BaseType

    func getLinkedNode(&self) : *mut ASTNode

}