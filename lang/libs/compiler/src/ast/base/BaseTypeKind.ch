public enum BaseTypeKind {

    Any,
    Array,
    Struct,
    Union,

    // bool, char and uchar are llvm / c intN types, however
    // we do not make them intN because they must not satisfy each other or any other type
    Bool,

    Double,
    Float,
    LongDouble,

    Complex,

    Float128,
    Function,
    CapturingFunction,
    Generic,
    IntN,
    Pointer,
    Reference,
    Linked,
    String,
    Literal,
    Dynamic,
    Void,

    ExpressionType,
    NullPtr,
    ExpressiveString,

    Unknown

}