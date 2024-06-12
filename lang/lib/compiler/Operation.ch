@scope:lexer
enum Operation {
    // Grouping and scope resolution operators
    Grouping,
    ScopeResolutionUnary,
    ScopeResolutionBinary,

    // Function call, subscript, structure member, structure pointer member
    FunctionCall,
    Subscript,
    StructureMember,
    StructurePointerMember,

    // Postfix increment and decrement
    PostfixIncrement,
    PostfixDecrement,

    // Unary operators
    LogicalNegate,
    OnesComplement,
    UnaryPlus,
    UnaryMinus,
    PrefixIncrement,
    PrefixDecrement,
    Indirection,
    AddressOf,
    Sizeof,
    TypeConversion,

    // Multiplicative operators
    Division,
    Multiplication,
    Modulus,

    // Additive operators
    Addition,
    Subtraction,

    // Shift operators
    LeftShift,
    RightShift,

    // Relational operators
    GreaterThan,
    GreaterThanOrEqual,
    LessThan,
    LessThanOrEqual,

    // Equality operators
    IsEqual,
    IsNotEqual,

    // Bitwise AND
    BitwiseAND,

    // Bitwise exclusive OR
    BitwiseXOR,

    // Bitwise inclusive OR
    BitwiseOR,

    // Logical AND
    LogicalAND,

    // Logical OR
    LogicalOR,

    // Conditional operator
    Conditional,

    // Assignment operators
    Assignment,
    AddTo,
    SubtractFrom,
    MultiplyBy,
    DivideBy,
    ModuloBy,
    ShiftLeftBy,
    ShiftRightBy,
    ANDWith,
    ExclusiveORWith,
    InclusiveORWith
};