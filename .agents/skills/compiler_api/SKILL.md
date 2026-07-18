---
name: Compiler API
description: The Chemical compiler API bindings in lang/libs/compiler/ and the C++ AST node hierarchy — ASTNode, Value, BaseType, ASTAllocator, and their unsafe casting methods.
---

# Compiler API

This document covers two aspects:
1. The **AST type hierarchy** in the C++ compiler — `ASTNode`, `Value`, `BaseType`, and their relationships
2. The **compiler API bindings** in `lang/libs/compiler/` — Chemical code that wraps the C++ API for plugin development

## Part 1: C++ AST Type Hierarchy

The AST (Abstract Syntax Tree) has three base classes: `ASTNode`, `Value`, and `BaseType`. All three inherit from `ASTAny`.

### The `ASTAny` Base Class

```cpp
// ast/base/ASTAny.h
class ASTAny {
public:
    virtual ASTAnyKind any_kind() = 0;  // Returns Value, Node, or Type
    ASTNode* get_ref_linked_node();
    std::string representation();
    // LLVM type methods (only in COMPILER_BUILD)
};
```

`ASTAnyKind` can be:
- `ASTAnyKind::Value` — the object is a `Value*`
- `ASTAnyKind::Node` — the object is an `ASTNode*`
- `ASTAnyKind::Type` — the object is a `BaseType*`

### ASTNode — Statements and Declarations

```cpp
// ast/base/ASTNode.h
class ASTNode : public ASTAny {
    ASTNodeKind nodeKind;           // The node discriminator
    ASTNode* parent_node;           // Parent in the AST tree
    SourceLocation encoded_location; // Source location (file:line:col)
    // ...
    
    inline ASTNodeKind kind() const noexcept { return nodeKind; }
    
    // Smart casting — checked in DEBUG, fast in release:
    ASTNode* as_function_unsafe();           // CHECK_COND(kind == FunctionDecl)
    StructDefinition* as_struct_def_unsafe();// CHECK_COND(kind == StructDecl)
    ImplDefinition* as_impl_def_unsafe();    // CHECK_COND(kind == ImplDecl)
    // ... and 50+ more
};
```

**ASTNodeKind** values include (from `ast/base/ASTNodeKind.h`):
- `FileScope`, `Scope`, `NamespaceDecl`
- `FunctionDecl`, `FuncPrototype`, `MultiFunctionNode`
- `StructDecl`, `UnionDef`, `VariantDecl`, `EnumDecl`
- `InterfaceDecl`, `ImplDecl`
- `VarInitStmt`, `AssignStmt`, `ReturnStmt`, `BreakStmt`, `ContinueStmt`
- `IfStmt`, `WhileLoopStmt`, `DoWhileLoopStmt`, `ForLoopStmt`, `ForInLoopStmt`
- `SwitchStmt`, `TryCatchStmt`, `ThrowStmt`
- `ImportStmt`, `ExportStmt`, `TypealiasStmt`, `UsingStmt`
- `GenericFuncDecl`, `GenericStructDecl`, `GenericVariantDecl`, `GenericImplDecl`, etc.
- `ValueNode`, `ValueWrapperNode`, `AccessChainNode`
- `AnnotableNode`, `CapturedVariable`, `UnreachableStmt`

**Unsafe Casting Pattern:**

```cpp
// All as_*_unsafe() methods follow this pattern:
inline FunctionDeclaration* as_function_unsafe() {
    CHECK_COND(kind() == ASTNodeKind::FunctionDecl);
    return static_cast<FunctionDeclaration*>(this);
}
```

`CHECK_COND` is a debug-only assertion that verifies the kind matches. In release builds, it's a no-op — just a `static_cast`. This is why debugging reveals cast errors that release builds silently ignore.

### Value — Expressions and Values

```cpp
// ast/base/Value.h
class Value : public ASTAny {
    ValueKind valueKind;            // The value discriminator
    BaseType* type;                 // The resolved type of this value
    // ...
    
    inline ValueKind kind() const noexcept { return valueKind; }
    inline ValueKind val_kind() const noexcept { return valueKind; }
    
    // Runtime type checking: (all inline bool methods)
    bool is_intn() const;           // Kind == ValueKind::IntN
    bool is_float() const;          // ValueKind::Float
    bool is_bool() const;           // ValueKind::Bool
    bool is_string() const;         // ValueKind::String
    bool is_struct() const;         // ValueKind::StructValue
    bool is_identifier() const;     // ValueKind::Identifier
    bool is_func_call() const;      // ValueKind::FunctionCall
    bool is_expression() const;     // ValueKind::Expression
    bool is_access_chain() const;   // ValueKind::AccessChain
    bool is_array_value() const;    // ValueKind::ArrayValue
    // ... and many more
    
    // Unsafe casts — same CHECK_COND pattern:
    inline Expression* as_expression_unsafe() {
        CHECK_COND(kind() == ValueKind::Expression);
        return static_cast<Expression*>(this);
    }
    
    inline StructValue* as_struct_value_unsafe() {
        CHECK_COND(kind() == ValueKind::StructValue);
        return static_cast<StructValue*>(this);
    }
    
    inline FunctionCall* as_func_call_unsafe() {
        CHECK_COND(kind() == ValueKind::FunctionCall);
        return static_cast<FunctionCall*>(this);
    }
    
    inline AccessChain* as_access_chain_unsafe() {
        CHECK_COND(kind() == ValueKind::AccessChain);
        return static_cast<AccessChain*>(this);
    }
    
    inline VariableIdentifier* as_identifier_unsafe() {
        CHECK_COND(kind() == ValueKind::Identifier);
        return static_cast<VariableIdentifier*>(this);
    }
    
    inline CastedValue* as_casted_value_unsafe() {
        CHECK_COND(kind() == ValueKind::CastedValue);
        return static_cast<CastedValue*>(this);
    }
    
    // ... and 30+ more
    
    // Key virtual methods:
    virtual Value* evaluated_value(InterpretScope& scope);   // Evaluate at comptime
    virtual Value* scope_value(InterpretScope& scope);       // Copy for scope storage
    virtual BaseType* getType();                             // Resolved type
    virtual bool set_value(InterpretScope& scope, Value* value, Operation op, SourceLocation loc);
};
```

**ValueKind** values include:
- `IntN` — integer literal
- `Float` — float literal
- `Double` — double literal
- `Bool` — boolean
- `String` — string literal
- `Char` — character literal
- `NullValue` — null pointer
- `StructValue` — struct literal `{ field: val }`
- `ArrayValue` — array literal `[1, 2, 3]`
- `Identifier` — variable reference
- `FunctionCall` — function call
- `Expression` — binary/unary operation (`a + b`)
- `AccessChain` — member access (`a.b.c`)
- `IndexOperator` — index access (`arr[i]`)
- `CastedValue` — type cast (`val as Type`)
- `DereferenceValue` — pointer deref (`*ptr`)
- `AddrOfValue` — address-of (`&raw mut`)
- `ReferenceOfValue` — reference (`&mut`)
- `LambdaFunction` — lambda expression
- `IsValue` — `is` operator
- `PatternMatchExpr` — pattern matching
- `NegativeValue`, `NotValue`, `BitwiseNot` — unary operators
- `SizeOfValue`, `AlignOfValue` — type size/align queries
- `NewValue`, `NewTypedValue` — allocation
- `WrapValue`, `ComptimeValue` — comptime wrappers
- `ExpressiveString` — backtick template strings
- `IncDecValue` — increment/decrement

### BaseType — Type Definitions

```cpp
// ast/base/BaseType.h
class BaseType : public ASTAny {
    // ...
    inline BaseTypeKind kind() const noexcept;
    
    // Runtime type checking:
    bool is_pointer() const;
    bool is_reference() const;
    bool is_int() const;       // BaseTypeKind::IntN
    bool is_float() const;
    bool is_bool() const;
    bool is_string() const;
    bool is_void() const;
    bool is_function() const;
    bool is_struct() const;    // BaseTypeKind::StructType
    bool is_array() const;
    bool is_generic() const;   // Generic type parameter
    bool is_linked() const;    // Linked to a concrete type
    bool is_dynamic() const;
    // ... and many more
    
    // Unsafe casts:
    inline LinkedType* as_linked_type_unsafe();
    inline PointerType* as_pointer_type_unsafe();
    inline ReferenceType* as_reference_type_unsafe();
    inline IntNType* as_intn_type_unsafe();
    inline FunctionType* as_function_type_unsafe();
    inline GenericType* as_generic_type_unsafe();
    inline StructType* as_struct_type_unsafe();
    inline ArrayType* as_array_type_unsafe();
    inline StringType* as_string_type_unsafe();
    inline BoolType* as_bool_type_unsafe();
    inline VoidType* as_void_type_unsafe();
    // ... and 20+ more
    
    // Key methods:
    virtual BaseType* pure_type(ASTAllocator& allocator);    // Resolve to concrete type
    virtual bool satisfies(BaseType* other);                  // Type compatibility
    virtual bool is_same(BaseType* other);                    // Exact type equality
    virtual MembersContainer* get_members_container();        // Get struct/variant members
};
```

**BaseTypeKind** values include:
- `IntN`, `Float`, `Double`, `Bool`, `Char`, `Void`
- `Pointer`, `Reference`, `Array`
- `StructType`, `UnionType`, `FunctionType`
- `LinkedType` — linked to a `StructDefinition`/`InterfaceDefinition`/etc.
- `GenericType` — a generic parameter `T`
- `StringType`, `LiteralType`, `ExprStringType`
- `AnyType`, `RuntimeType`, `MaybeRuntimeType`
- `DynamicType`, `IfType`
- `EnumType`
- `LongDouble`, `Float128`, `Complex`

### ASTAllocator — Arena Allocation

```cpp
// ast/base/ASTAllocator.h
class ASTAllocator final : public BatchAllocator {
    std::vector<ASTAny*> ptr_storage;         // For virtual destructor calls
    std::vector<ASTCleanupFunction> cleanup_fns; // Custom cleanup functions
    
    template<typename T>
    FORCE_INLINE T* allocate() {
        static_assert(std::is_base_of<ASTAny, T>::value);
        return (T*) allocate_size(sizeof(T), alignof(T));
    }
    
    char* allocate_size(std::size_t obj_size, std::size_t alignment);
    char* allocate_str(const char* data, std::size_t len);  // Allocate string storage
    void clear();   // Free everything, ready for reuse
    ~ASTAllocator(); // Destructs all stored pointers
};
```

**Key patterns:**
- `allocator.allocate<T>()` — creates a new T on the arena (no `new`/`delete`)
- `allocator.allocate_str(data, len)` — allocates string memory on the arena
- `file_allocator.clear()` — called after each symres pass to reuse memory
- `ptr_storage` ensures virtual destructors are called for ASTAny objects

### The `CHECK_COND` Debug Macro

```cpp
#ifdef DEBUG
#define CHECK_COND(cond) \
    if(!(cond)) { \
        std::cerr << "FATAL: " #cond " failed in " << __FILE__ << ":" << __LINE__ << std::endl; \
        std::terminate(); \
    }
#else
#define CHECK_COND(cond) ((void)0)
#endif
```

This means:
- **Debug builds**: Every unsafe cast checks the discriminator — catches bugs early
- **Release builds**: Just a static_cast — maximum performance
- If you see a "FATAL" message in CI, it means an `as_*_unsafe()` cast was used on the wrong type

### Type Hierarchy Summary

```
ASTAny
├── ASTNode (ASTNodeKind) — Statements and declarations
│   ├── FunctionDeclaration, StructDefinition, VariantDefinition
│   ├── IfStatement, WhileLoop, ReturnStatement, VarInitStatement
│   ├── Namespace, Scope, ImportStatement
│   ├── GenericFuncDecl, GenericStructDecl
│   └── ValueNode, ValueWrapperNode (wraps a Value in an ASTNode)
│
├── Value (ValueKind) — Expressions and values
│   ├── IntNumValue, FloatValue, BoolValue, StringValue
│   ├── StructValue, ArrayValue, NullValue
│   ├── VariableIdentifier, FunctionCall, Expression
│   ├── AccessChain, CastedValue, LambdaFunction
│   └── PointerValue, DereferenceValue, ReferenceOfValue
│
└── BaseType (BaseTypeKind) — Type definitions
    ├── LinkedType, PointerType, ReferenceType
    ├── IntNType, FloatType, DoubleType, BoolType, VoidType
    ├── StructType, UnionType, FunctionType, ArrayType
    ├── GenericType, StringType, AnyType
    └── DynamicType, IfType, MaybeRuntimeType
```

## Part 2: Compiler API Bindings (for Plugins)

The compiler API bindings are in `lang/libs/compiler/src/`. These are Chemical source files that expose compiler functionality via CBI (Compiler Binding Interface).

### Key Files

| File | Purpose |
|------|---------|
| `ASTBuilder.ch` | Create AST nodes: types, values, statements, functions |
| `BatchAllocator.ch` | Arena allocator wrapper for plugins |
| `Lexer.ch` | Tokenize Chemical source |
| `Parser.ch` | Parse Chemical source into AST |
| `SymbolResolver.ch` | Resolve symbols in AST |
| `SymbolTable.ch` | Declare and look up symbols |
| `SourceProvider.ch` | Source file access |
| `ASTDiagnoser.ch` | Report errors/warnings from plugins |
| `Token.ch` | Token data structure |
| `Position.ch` | Source position |
| `Operation.ch` | Operation types |
| `ChemicalTokenType.ch` | Token type enum |
| `AccessSpecifier.ch` | Public/private/internal access specifiers |
| `ASTVisitor.ch` | Base AST visitor |

See the [CBI Plugin API](./.agents/skills/cbi_plugin_api/SKILL.md) skill for how to use these bindings.

## Related Skills

- **CBI Plugin API** (`.agents/skills/cbi_plugin_api/SKILL.md`) — How to use the compiler API bindings in a plugin
- **Symbol Resolution** (`.agents/skills/symres/SKILL.md`) — How symbols are resolved in the AST
- **Parser Internals** (`.agents/skills/parser_internals/SKILL.md`) — How the parser creates AST nodes
- **Type Verification** (`.agents/skills/type_verification/SKILL.md`) — How types are verified
- **Performance** (`.agents/skills/performance/SKILL.md`) — Arena allocation patterns
