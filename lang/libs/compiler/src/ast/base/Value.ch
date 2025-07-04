public struct Value {

    func getEncodedLocation(&self) : ubigint

    func getKind(&self) : ValueKind

    func link(&self, ptr_ref : **mut Value, type : *mut BaseType, resolver : *mut SymbolResolver) : bool

    func getLinkedNode(&self) : *mut ASTNode

}