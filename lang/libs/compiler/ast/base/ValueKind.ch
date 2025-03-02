enum ValueKind {

    // integer types
    // signed integer types
    Char,
    Short,
    Int,
    Long,
    BigInt,
    Int128,
    // unsigned integer types
    UChar,
    UShort,
    UInt,
    ULong,
    UBigInt,
    UInt128,
    // unsigned integers end
    NumberValue,
    // integer number values end here

    Float,
    Double,
    Bool,
    String,
    Expression,
    ArrayValue,
    StructValue,
    LambdaFunc,
    IfValue,
    SwitchValue,
    LoopValue,
    NewTypedValue,
    NewValue,
    PlacementNewValue,
    IncDecValue,
    IsValue,
    DereferenceValue,
    RetStructParamValue,
    AccessChain,
    CastedValue,
    Identifier,
    IndexOperator,
    FunctionCall,
    NegativeValue,
    NotValue,
    NullValue,
    SizeOfValue,
    SymResValue,
    UnsafeValue,
    ComptimeValue,
    AlignOfValue,
    VariantCase,
    AddrOfValue,
    PointerValue,
    WrapValue,

    TypeContainingValue

}