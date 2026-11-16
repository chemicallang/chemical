public struct BaseType {

    func getKind(&self) : BaseTypeKind

    // The type checker's own value-vs-type rule (`assignment` allows the same
    // implicit conversions a field assignment allows). Fails open on null.
    func satisfies_value(&self, value : *mut Value, assignment : bool) : bool

    // Type spelling for diagnostics ("int", "std::string_view"). Writes at most
    // cap-1 bytes plus a terminator; returns the bytes written.
    func representation(&self, out : *mut char, cap : ubigint) : ubigint

}