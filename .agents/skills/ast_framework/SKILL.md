---
name: AST Framework
description: The C++ AST node framework of the Chemical compiler — the ASTAny/ASTNode/Value/BaseType hierarchy, node/value/type kind enums and range predicates, the visitor dispatch framework, the arena allocator, TypeBuilder canonicalization, member containers, and cross-reference resolution. Load when adding or modifying AST nodes, writing compiler passes/codegen visitors, debugging kind mismatches or arena/lifetime issues, or extending the compiler API bindings.
---

# AST Framework

This skill documents the **C++ AST framework** that every compiler pass, backend, and
plugin builds on. It covers how nodes are structured, discriminated, allocated, traversed,
and cross-linked.

> **Companion skill:** The [Compiler API skill](../compiler_api/SKILL.md) documents the
> *binding* side — `lang/libs/compiler/` Chemical wrappers and the `as_*_unsafe()` cast
> surface. This document focuses on the C++ internals; where casting and the binding enum
> mirrors are relevant they are referenced, not repeated.

## Overview: A Single Tree, Three Roots

The compiler's syntax representation is a heterogeneous tree. Every object in it descends
from one abstract base class, `ASTAny` (`ast/base/ASTAny.h:27`), which exposes a single
virtual discriminator:

```cpp
class ASTAny {
public:
    virtual ASTAnyKind any_kind() = 0;          // ast/base/ASTAny.h:33
    ASTNode* get_ref_linked_node();             // ast/base/ASTAny.h:38
    std::string representation();               // ast/base/ASTAny.h:60
    virtual ~ASTAny();                          // ast/base/ASTAny.h:62
};
```

`any_kind()` (`ast/base/ASTAnyKind.h:10`) returns exactly one of three values, and there
are exactly three concrete roots:

```
ASTAny  (ast/base/ASTAny.h:27)
├── ASTNode   any_kind() == ASTAnyKind::Node   — statements, declarations, scopes, params
│              discriminated by ASTNodeKind     (ast/base/ASTNodeKind.h:14)
├── Value     any_kind() == ASTAnyKind::Value  — expressions, literals, identifiers, calls
│              discriminated by ValueKind       (ast/base/ValueKind.h:10)
└── BaseType  any_kind() == ASTAnyKind::Type   — type descriptors
               discriminated by BaseTypeKind     (ast/base/BaseTypeKind.h:14)
```

`ASTAnyKind` is a `uint8_t` enum: `Value, Type, Node` (`ast/base/ASTAnyKind.h:10-15`).

Each root declares `any_kind() final` and returns its constant, so the three trees never
overlap:

- `ASTNode::any_kind()` → `ASTAnyKind::Node` (`ast/base/ASTNode.h:96`)
- `Value::any_kind()` → `ASTAnyKind::Value` (`ast/base/Value.h:105`)
- `BaseType::any_kind()` → `ASTAnyKind::Type` (`ast/base/BaseType.h:76`)

`ASTAny::representation()` (`ast/base/ASTAny.cpp:34`) and `ASTAny::get_ref_linked_node()`
(`ast/base/ASTAny.cpp:47`) switch on `any_kind()` to dispatch into the correct root without
RTTI. This is the only place the three roots are treated uniformly.

> **Gotcha:** not every `ASTAny` is an `ASTNode`/`Value`/`BaseType`. `WhereClause`
> (`ast/structures/WhereClause.h:40`) derives directly from `ASTAny` and returns
> `ASTAnyKind::Node` while *not* being an `ASTNode` (it has no `kind()`, no `parent()`, and
> is not handled by the visitors). Do not treat every `ASTAnyKind::Node` as an `ASTNode`.

The per-file tree root is `ASTUnit` (`ast/base/ASTUnit.h:15`), which owns a `FileScope` and
is what the parser produces. `FileScope`/`ModuleScope` are themselves `ASTNode`s
(`ASTNodeKind::FileScope`/`ModuleScope`) so scope walking and ancestor lookups work
uniformly.

## Key Files

| File | Purpose |
|------|---------|
| `ast/base/ASTAny.h` / `.cpp` | Abstract base; `any_kind()`, `representation()`, `get_ref_linked_node()`, LLVM hooks |
| `ast/base/ASTAnyKind.h` | `ASTAnyKind` enum (`Value`/`Type`/`Node`) |
| `ast/base/ASTNode.h` / `.cpp` | Node base: `_kind`, `_parent`, `_location`; `is*`/`as*()` predicates; codegen virtuals |
| `ast/base/ASTNodeKind.h` | `ASTNodeKind` enum — one value per node class |
| `ast/base/Value.h` / `.cpp` | Value base: `kind()`, `getType()`, comptime evaluation, LLVM value virtuals |
| `ast/base/ValueKind.h` | `ValueKind` enum — one value per value class |
| `ast/base/BaseType.h` / `.cpp` | Type base: `kind()`, `canonical()`, `satisfies()`, linked-node accessors |
| `ast/base/BaseTypeKind.h` | `BaseTypeKind` enum |
| `ast/base/ASTAllocator.h` / `.cpp` | Arena that also tracks pointers for virtual destruction |
| `ast/base/BatchAllocator.h` / `.cpp` (impl in `ASTAllocator.cpp`) | Pointer-bump batch arena |
| `ast/base/TypeBuilder.h` / `.cpp` | Flyweight cache of primitive types + shared `nullValue`, `unresolvedDecl` |
| `ast/base/TypeLoc.h` | `BaseType*` + `SourceLocation` pair (types carry no location themselves) |
| `ast/base/InterpretScope.h` / `.cpp` | Interpreter scope: value map, parent chain, move semantics |
| `ast/base/GlobalInterpretScope.h` / `.cpp` | Global interpreter state + type builder + diagnoser |
| `ast/base/ExtendableMembersContainerNode.h` | Base of struct/union/variant/interface |
| `ast/base/LoopASTNode.h` | Base of all loops (owns `Scope body`) |
| `ast/base/DebugCast.h` | `CHECK_CAST` / `CHECK_COND` debug-only ablation macros |
| `ast/base/ChildResolution.h` | `provide_child(...)` cross-reference helpers |
| `ast/base/ast_fwd.h` | Forward declarations of (almost) every node/value/type class |
| `ast/base/ScopeValueMap.h` | Flat-vector scope storage for the interpreter |
| `ast/base/ChainPart.h`, `LocatedIdentifier.h`, `AccessSpecifier.h` | Small support types |
| `preprocess/visitors/NonRecursiveVisitor.h` | CRTP dispatch base for all visitors |
| `preprocess/visitors/RecursiveVisitor.h` | CRTP base that adds deep traversal |

## Kind Taxonomy and Range Predicates

All four enums are `uint8_t` `enum class`es. Their values mirror each other across the C++
compiler and the CBI binding `.ch` files (see the [CBI Plugin API skill](../cbi_plugin_api/SKILL.md)).

### `ASTNodeKind` grouping

`ASTNodeKind` (`ast/base/ASTNodeKind.h:14-93`) lists values in blank-line-separated groups
that are semantically meaningful:

| Group (contiguous run) | Values |
|------------------------|--------|
| Statement / value-carrying nodes | `AssignmentStmt … InlineAsmStmt` (`:16-45`) |
| Type declarations & members | `EnumDecl … UnnamedUnion` (`:47-60`) |
| Scopes | `Scope, Block, UnsafeBlock` (`:62-64`) |
| Params / captured / pattern vars | `FunctionParam … PatternMatchId` (`:66-72`) |
| Concrete struct/union runtime types | `StructType, UnionType` (`:74-75`) |
| Generic declarations | `GenericFuncDecl … GenericTypeDecl` (`:77-83`) |
| File / module scope | `FileScope, ModuleScope` (`:85-86`) |
| Codegen/macro helpers | `EmbeddedNode, ExportStmt, ChildrenMapNode` (`:88-90`) |
| Placeholder | `UnresolvedDecl` (`:92`) |

The ordering is convention, not a numeric contract — the helper predicates test explicit
sets, not `>=`/`<=` ranges. The one guarantee that matters is that **appending** at the end
does not disturb existing values; inserting in the middle shifts every later value and
breaks the CBI side.

### Range predicates (`ASTNode.h`)

`ASTNode` ships `static constexpr bool is*()` helpers that accept an `ASTNodeKind` so they
can be used on a kind before a node exists. Single-kind helpers are trivial
(`isFunctionDecl` at `ast/base/ASTNode.h:505`), while multi-kind helpers encode the
categories:

```cpp
static inline constexpr bool isLoopASTNode(ASTNodeKind k) {          // ast/base/ASTNode.h:577
    return k == ASTNodeKind::WhileLoopStmt || k == ASTNodeKind::DoWhileLoopStmt
        || k == ASTNodeKind::ForLoopStmt || k == ASTNodeKind::ForInLoopStmt
        || k == ASTNodeKind::LoopBlock;
}
static inline constexpr bool isMembersContainer(ASTNodeKind k) {     // ast/base/ASTNode.h:581
    return k == ASTNodeKind::StructDecl || k == ASTNodeKind::UnionDecl
        || k == ASTNodeKind::VariantDecl || k == ASTNodeKind::InterfaceDecl
        || k == ASTNodeKind::ImplDecl;
}
static inline constexpr bool isAnyStructMember(ASTNodeKind k) {      // ast/base/ASTNode.h:585
    return k == ASTNodeKind::StructMember || k == ASTNodeKind::UnnamedStruct
        || k == ASTNodeKind::UnnamedUnion;
}
static inline constexpr bool isBaseDefMember(ASTNodeKind k) {        // ast/base/ASTNode.h:589
    return isAnyStructMember(k) || k == ASTNodeKind::VariantMember;
}
static inline constexpr bool isStoredStructType(ASTNodeKind k) {     // ast/base/ASTNode.h:593
    return k == ASTNodeKind::StructDecl || k == ASTNodeKind::UnionDecl
        || k == ASTNodeKind::VariantDecl || k == ASTNodeKind::VariantMember
        || k == ASTNodeKind::InterfaceDecl || k == ASTNodeKind::UnnamedStruct
        || k == ASTNodeKind::UnnamedUnion;
}
static inline constexpr bool isStoredStructDecl(ASTNodeKind k) {     // ast/base/ASTNode.h:597
    return k == ASTNodeKind::StructDecl || k == ASTNodeKind::UnionDecl
        || k == ASTNodeKind::VariantDecl || k == ASTNodeKind::InterfaceDecl;
}
static inline constexpr bool isAnnotableNode(ASTNodeKind k) {        // ast/base/ASTNode.h:601
    return k == ASTNodeKind::UsingStmt || k == ASTNodeKind::VarInitStmt
        || k == ASTNodeKind::NamespaceDecl || isBaseDefMember(k)
        || isFunctionDecl(k) || isMembersContainer(k);
}
```

These are consumed by the nullable casts, e.g. `as_members_container()` returns
`(MembersContainer*) this` only when `isMembersContainer(kind())`
(`ast/base/ASTNode.h:633`), and by `get_members_container()` /
`get_master_members_container()` (`ast/base/ASTNode.cpp:293` / `:308`) which also unwrap
type aliases and generic masters.

### Value and type predicates

`Value` has single-kind predicates (`isIntN`, `isFloat`, `isStructValue`, …) declared at
`ast/base/Value.h:749-875`, plus the multi-kind `isChainValue`:

```cpp
static constexpr inline bool isChainValue(ValueKind k) {             // ast/base/Value.h:793
    return k == ValueKind::Identifier || k == ValueKind::FunctionCall
        || k == ValueKind::IndexOperator || k == ValueKind::AccessChain;
}
```

`BaseType` has kind predicates at `ast/base/BaseType.h:232-241` and category helpers:

```cpp
static inline constexpr bool is_pointer(BaseTypeKind k) {            // ast/base/BaseType.h:232
    return k == BaseTypeKind::Pointer || k == BaseTypeKind::String;
}
static inline constexpr bool is_pointer_or_ref(BaseTypeKind k) {     // ast/base/BaseType.h:239
    return k == BaseTypeKind::Reference || is_pointer(k);
}
static inline constexpr bool isIntFloatOrBool(BaseTypeKind k) {      // ast/base/BaseType.h:461
    return k == BaseTypeKind::IntN || k == BaseTypeKind::Bool
        || k == BaseTypeKind::Double || k == BaseTypeKind::Float;
}
static inline constexpr bool isPrimitiveType(BaseTypeKind k) {       // ast/base/BaseType.h:465
    return isIntFloatOrBool(k) || k == BaseTypeKind::Pointer;
}
static inline constexpr bool isLoadableReferencee(BaseTypeKind k) {  // ast/base/BaseType.h:469
    return isPrimitiveType(k) || k == BaseTypeKind::Function
        || k == BaseTypeKind::Literal;
}
```

There is **no** `isValue`/`isNode`/`isType` predicate on the roots; the root category comes
from `any_kind()` and the concrete category from the per-root `is*` helpers.

### Full enum inventories

| Enum | Values | Source |
|------|--------|--------|
| `ASTAnyKind` | `Value, Type, Node` | `ast/base/ASTAnyKind.h:10` |
| `ASTNodeKind` | 69 values, statements → declarations → scopes → params → generic decls → scopes → helpers | `ast/base/ASTNodeKind.h:14` |
| `ValueKind` | 51 values: literals, expressions, chain values, comptime wrappers | `ast/base/ValueKind.h:10` |
| `BaseTypeKind` | 28 values: primitives, composite types, wrappers | `ast/base/BaseTypeKind.h:14` |

`ValueKind` includes the literals (`IntN`, `Float`, `Double`, `Bool`, `String`), the chain
values (`Identifier`, `FunctionCall`, `IndexOperator`, `AccessChain`), operators/expressions
(`Expression`, `CastedValue`, `DereferenceValue`, `AddrOfValue`, `ReferenceOfValue`), and
comptime wrappers (`ExpressiveString`, `RawLiteral`, `IfValue`/`SwitchValue`/`LoopValue`)
(`ast/base/ValueKind.h:10-67`).

`BaseTypeKind` spans primitives (`IntN`, `Bool`, `Float`, `Double`, `Void`), composites
(`Pointer`, `Reference`, `Array`, `Struct`, `Union`, `Function`, `CapturingFunction`), and
wrappers (`Linked`, `Generic`, `Literal`, `Runtime`, `MaybeRuntime`, `Dynamic`)
(`ast/base/BaseTypeKind.h:14-53`). Note `:21-22`: `bool`, `char`, and `uchar` use integer
storage in LLVM/C but are deliberately not `IntN` so they do not satisfy one another.

### Casting and debug checks

Unsafe casts are of the form `CHECK_CAST(kind)` then a `static_cast`. `CHECK_CAST` is a
debug-only `abort()` (`ast/base/DebugCast.h:9`); in release it expands to nothing, so an
`as_*_unsafe()` on the wrong kind is undefined behavior that only debug builds catch. The
[Compiler API skill](../compiler_api/SKILL.md) enumerates the cast methods; this document
only notes the mechanism.

## Directory Inventory

| Directory | Contents | Representative classes | Size |
|-----------|----------|------------------------|------|
| `ast/base` | Framework only: roots, kinds, allocators, scopes, `TypeLoc`, member-container bases | `ASTAny`, `ASTNode`, `Value`, `BaseType`, `ASTAllocator`, `BatchAllocator`, `TypeBuilder`, `InterpretScope`, `GlobalInterpretScope`, `ExtendableMembersContainerNode`, `LoopASTNode`, `UnresolvedDecl` support | 30 `.h`, 9 `.cpp` |
| `ast/values` | All `Value` subclasses (expressions, literals, calls, chains, comptime wrappers) | `IntNumValue`, `StringValue`, `StructValue`, `ArrayValue`, `VariableIdentifier`, `FunctionCall`, `Expression`, `AccessChain`, `CastedValue`, `LambdaFunction`, `AddrOfValue`, `ReferenceOfValue`, `DereferenceValue`, `SizeOfValue`, `ExpressiveString`, `NewValue` | 54 `.h`, 36 `.cpp` |
| `ast/types` | All `BaseType` subclasses | `IntNType` (+ `I8Type`, `IntType`, …), `BoolType`, `FloatType`, `DoubleType`, `VoidType`, `PointerType`, `ReferenceType`, `ArrayType`, `StructType`, `UnionType`, `LinkedType`, `GenericType`, `FunctionType`, `CapturingFunctionType`, `DynamicType`, `StringType`, `AnyType`, `LiteralType`, `RuntimeType`, `MaybeRuntimeType`, `IfType`, `NullPtrType` | 28 `.h`, 15 `.cpp` |
| `ast/statements` | Statement-like `ASTNode`s and value wrappers | `VarInitStatement`, `AssignStatement`, `ReturnStatement`, `BreakStatement`, `ContinueStatement`, `SwitchStatement`, `ThrowStatement`, `TypealiasStatement`, `UsingStmt`, `ImportStatement`, `AccessChainNode`, `ValueNode`, `ValueWrapperNode`, `IncDecNode`, `InlineAsmStatement`, `UnresolvedDecl`, `DestructStmt`, `EmbeddedNode` | 25 `.h`, 13 `.cpp` |
| `ast/structures` | Declarations, scopes, containers, control-flow structures, generics | `FunctionDeclaration`, `StructDefinition`, `UnionDef`, `VariantDefinition`, `EnumDeclaration`, `InterfaceDefinition`, `ImplDefinition`, `Namespace`, `Scope`, `BlockScope`, `FileScope`, `ModuleScope`, `IfStatement`, `WhileLoop`, `ForLoop`, `ForInLoop`, `TryCatch`, `MembersContainer`, `BaseDefMember`, `StructMember`, `Generic*Decl`, `WhereClause` | 47 `.h`, 25 `.cpp` |
| `ast/utils` | Free helpers, operation enum, intrinsics, target API | `Operation` (`Operation.h`), `ASTUtils`, `GenericUtils`, `GlobalFunctions` (intrinsics like `InterpretExprPrintLn`), `IffyConditional`, `TargetAPI` | 5 `.h`, 3 `.cpp` |

Category notes:

- **`ast/values` vs `ast/statements`:** an expression that the parser needs to treat as a
  statement is wrapped in a `ValueWrapperNode` (ASTNodeKind::ValueWrapper,
  `ast/statements/ValueWrapperNode.h:8`) or `ValueNode` (`ast/values/ValueNode.h:21`).
  `ValueWrapperNode` forwards `code_gen` to `value->llvm_value(...)`
  (`ast/statements/ValueWrapperNode.h:29`), whereas `ValueNode` is only a linking/extraction
  carrier and crashes if codegen runs on it (`ast/values/ValueNode.h:8-19`).
- **`ast/structures`** holds both declarations *and* control flow. Loops share
  `LoopASTNode` which owns `Scope body` (`ast/base/LoopASTNode.h:22-36`).
- **`WhereClause`** lives in `ast/structures` despite being an `ASTAny` rather than an
  `ASTNode` (`ast/structures/WhereClause.h:40`).

## Value vs ASTNode vs BaseType

| Root | Represents | Identity / payload | Examples |
|------|-----------|--------------------|----------|
| `ASTNode` | Anything with a position in the tree: statements, declarations, scopes, params. Has `_parent` and a `SourceLocation`. | `ASTNodeKind _kind`, `ASTNode* _parent`, `SourceLocation _location` (`ast/base/ASTNode.h:52-68`) | `VarInitStatement`, `FunctionDeclaration`, `StructDefinition`, `IfStatement` |
| `Value` | An *expression* that evaluates to something; carries a resolved `BaseType*`. | `ValueKind _kind`, `BaseType* _type`, `SourceLocation _location` (`ast/base/Value.h:54-70`) | `IntNumValue`, `VariableIdentifier`, `FunctionCall`, `Expression`, `StructValue` |
| `BaseType` | A type descriptor only. **No source location, no parent.** | `BaseTypeKind _kind` (`ast/base/BaseType.h:32-38`) | `IntNType`, `PointerType`, `LinkedType`, `GenericType` |

Rules of thumb:

- If it can be assigned to, called, or dereferenced, it is a `Value`.
- If it declares a name or controls evaluation order, it is an `ASTNode`.
- If it is asked "what type is X", the answer is a `BaseType*` reached via
  `Value::getType()` (`ast/base/Value.h:112`) or `ASTNode::getType()` /
  `known_type()` (`ast/base/ASTNode.cpp:210`, `ast/base/ASTNode.h:300`).

### `TypeLoc`: type + location

Because `BaseType` carries no location (it is reused/canonicalized — see the
`GlobalBaseType` comment at `ast/base/BaseType.h:786-799`), source-written types are stored
as `TypeLoc` (`ast/base/TypeLoc.h:11`) which pairs `BaseType const*` with a
`SourceLocation`:

```cpp
class TypeLoc {
    BaseType const* type_;
    SourceLocation loc_;
public:
    constexpr TypeLoc(BaseType const* type, SourceLocation loc) noexcept;      // :24
    constexpr operator BaseType*() const noexcept;                             // :34
    constexpr BaseType* operator->() const noexcept;                           // :39
    constexpr BaseType* getType() const noexcept;                              // :71
    constexpr SourceLocation encoded_location() const noexcept;                // :65
};
```

`TypeLoc` converts implicitly to `BaseType*`, so most code uses it like a pointer.
Fields such as `StructMember::type` (`ast/structures/StructMember.h:34`) and
`FunctionCall::generic_list` (`ast/values/FunctionCall.h:27`) are `TypeLoc`. To replace a
type during a visitor you must write back through the `TypeLoc` because the pointer is
`const` — see `RecursiveVisitor::VisitCastedValue` which reassigns
`casted->setType(...)` if the visited type changed (`preprocess/visitors/RecursiveVisitor.h:198-205`).

### `InterpretScope` / `GlobalInterpretScope` are part of `ast/base`

The AST interpreter state classes live in `ast/base` because they are coupled to the node
hierarchy (they hold `Value*` maps and evaluate nodes/values directly):

- `InterpretScope` (`ast/base/InterpretScope.h:40`): a per-function/per-block frame. Holds
  `ScopeValueMap values` (`:48`), `InterpretScope* parent` (`:53`), a
  `GlobalInterpretScope* global` (`:58`), a lightweight `ASTAllocator&` (`:64`), the
  function `returnValue`, loop-control flags, and `move_clear_source()` for move semantics
  (`:346`). It exposes `allocate<T>()` (`:176`), `declare/find_value`, `interpret(node)`,
  and `destroy_values()`.
- `GlobalInterpretScope` (`ast/base/GlobalInterpretScope.h:35`): also an `ASTDiagnoser`.
  Holds `OutputMode`, `TargetData`, the call stack, `LabBuildCompiler*`, `BackendContext*`,
  the `ASTAllocator&`, and the `TypeBuilder& typeBuilder` (`:95-100`).

`ScopeValueMap` (`ast/base/ScopeValueMap.h:32`) is a flat, insertion-ordered vector that
lazily builds a hash index past `INDEX_THRESHOLD = 12` (`:39`), avoiding a heap node per
variable in hot interpreter loops.

## Visitors and Dispatch

### There is no `accept()`; dispatch is a kind switch (CRTP)

Traversal is **not** the textbook virtual `accept(visitor)` pattern. Instead,
`NonRecursiveVisitor<Derived>` (`preprocess/visitors/NonRecursiveVisitor.h:13`) is a CRTP
base that switches on `kind()`:

```cpp
void VisitNodeNoNullCheck(ASTNode* node) {                          // NonRecursiveVisitor.h:535
    switch(node->kind()) {
        case ASTNodeKind::AssignmentStmt:
            static_cast<Derived*>(this)->VisitAssignmentStmt((AssignStatement*) node);
            return;
        case ASTNodeKind::FunctionDecl:
            static_cast<Derived*>(this)->VisitFunctionDecl((FunctionDeclaration*) node);
            return;
        // ... one case per ASTNodeKind
        default:
            CHEM_THROW_RUNTIME("UNHANDLED: node kind in non recursive visitor");  // :728
    }
}
```

It provides three parallel dispatchers:

| Dispatcher | Switch on | Line |
|------------|-----------|------|
| `VisitNodeNoNullCheck(ASTNode*)` | `ASTNodeKind` | `preprocess/visitors/NonRecursiveVisitor.h:535` |
| `VisitValueNoNullCheck(Value*)` | `ValueKind` | `preprocess/visitors/NonRecursiveVisitor.h:737` |
| `VisitTypeNoNullCheck(BaseType*)` | `BaseTypeKind` | `preprocess/visitors/NonRecursiveVisitor.h:903` |

Every concrete `Visit*()` has a default that forwards to a category hook, so derived
visitors override only what they need:

```cpp
inline void VisitCommonNode(ASTNode* node) { }      // NonRecursiveVisitor.h:19
inline void VisitCommonValue(Value* value) { }      // NonRecursiveVisitor.h:24
inline void VisitCommonType(BaseType* type) { }     // NonRecursiveVisitor.h:29
```

The `default:` cases throw in debug builds (`:728`, `:894`, `:988`) — a missing handler for
a newly added kind aborts early. The public entry points are `visit(ASTNode*)`,
`visit(Value*)`, `visit(BaseType*)`, `visit(TypeLoc&)`, `visit(Scope&)`
(`preprocess/visitors/NonRecursiveVisitor.h:1362-1386`); note these do **not** null-check.
Convenience `VisitNode()`/`VisitValue()`/`VisitType()` wrappers do null-check
(`:733`, `:899`, `:994`).

### `RecursiveVisitor` adds deep traversal

`RecursiveVisitor<Derived>` (`preprocess/visitors/RecursiveVisitor.h:117`) derives from the
non-recursive base and implements one `Visit*()` per node that walks children through
`visit_it(...)`:

```cpp
inline void visit_it(Value* value) {                                // RecursiveVisitor.h:136
    static_cast<Derived*>(this)->visit(value);   // re-enters kind switch
}
void VisitFunctionCall(FunctionCall* call) {                        // RecursiveVisitor.h:162
    visit_it(call->parent_val);
    for(auto& arg : call->generic_list) visit_it(arg);
    for(auto& val : call->values) visit_it(val);
}
```

`visit_it` bridges back into the `NonRecursiveVisitor` switch, so a derived class overriding
`VisitFunctionCall` **must** call the base implementation to keep recursing (or call
`visit_it` on children itself). `RecursiveVisitor` also carries the mutable reference
overloads `visit_it(T*&)` and `visit_it(BaseType*&, SourceLocation)` (`:140-148`) so passes
can rewrite child pointers and types in place.

### Concrete visitors (who traverses what)

| Visitor | Base | File | Role |
|---------|------|------|------|
| `RepresentationVisitor` | `NonRecursiveVisitor` | `preprocess/RepresentationVisitor.h:11` | AST → Chemical source text; used by `representation()` and the interpreter |
| `TopLevelDeclSymDeclare` | `NonRecursiveVisitor` | `compiler/symres/DeclareTopLevel.h:7` | Symres phase 1 — declare top-level symbols |
| `TopLevelLinkSignature` | `RecursiveVisitor` | `compiler/symres/LinkSignature.h:8` | Symres phase 2 — link signatures |
| `GenericInstantiationPass` | `RecursiveVisitor` | `compiler/symres/GenericInstantiationPass.h:15` | Symres phase 3 — collect/instantiate generics |
| `SymResLinkBody` | `NonRecursiveVisitor` | `compiler/symres/SymResLinkBody.h:20` | Symres phase 5 — resolve bodies |
| `GenericInstantiator` | `RecursiveVisitor` | `compiler/generics/GenericInstantiator.h:14` | Monomorphization |
| `TypeVerifier` | `RecursiveVisitor` | `compiler/typeverify/TypeVerify.h:10` | Second type-checking pass |
| `ToCAstVisitor` | `NonRecursiveVisitor` + `ASTDiagnoser` | `preprocess/2c/2cASTVisitor.h:30` | C translation |
| `CDestructionVisitor` | `SubVisitor` | `preprocess/2c/CDestructionVisitor.h:39` | C destructor scheduling |
| `CTopLevelDeclarationVisitor` | `NonRecursiveVisitor` + `SubVisitor` | `preprocess/2c/CTopLevelDeclVisitor.h:17` | C forward declarations |
| LSP analyzers | both bases | `server/analyzers/*.h` | Completion, document symbols, inlay hints |

> **No `VisitorContext` type exists.** A search for `VisitorContext` across the tree yields
> nothing. State is held as member fields on each visitor (as documented in the
> [Compiler API skill](../compiler_api/SKILL.md)).

### LLVM codegen does not use the visitor framework

The LLVM backend traverses by calling **virtual methods on the nodes themselves**, not via
`NonRecursiveVisitor`:

- `ASTNode::code_gen(Codegen&)` (`ast/base/ASTNode.h:385`), with the overload
  `code_gen(Codegen&, Scope*, unsigned)` for last-statement context (`:394`), plus
  `code_gen_declare` (`:368`) and `code_gen_external_declare` (`:378`).
- `Value::llvm_value(Codegen&, BaseType*)` (`ast/base/Value.h:493`) and
  `Value::llvm_pointer(Codegen&)` (`:485`).
- `BaseType::llvm_type(Codegen&)` and `ASTAny::llvm_type(Codegen&)` (`ast/base/ASTAny.h:45`).

`llvm_type` lives on `ASTAny` and is guarded by `#ifdef COMPILER_BUILD`
(`ast/base/ASTAny.h:14-55`), so free-standing TCC/plugin builds do not drag in LLVM.

## Arena Allocation (`ASTAllocator` / `BatchAllocator`)

AST nodes are **never** individually `new`ed or `delete`d. They come from a bump arena:

```cpp
class BatchAllocator {                                      // ast/base/BatchAllocator.h:16
    std::vector<char*> heap_memory;                         // :69
    std::size_t heap_batch_size;                            // :73
    std::size_t heap_offset;                                // :83
    std::shared_ptr<std::mutex> allocator_mutex;            // :88
    char* reserve_heap_storage();                           // :99
    char* object_heap_pointer(size_t obj_size, size_t alignment);  // :104
public:
    char* allocate_str(const char* data, size_t size);      // :54
    char* allocate_released_size(size_t obj_size, size_t alignment); // :49
};

class ASTAllocator final : public BatchAllocator {          // ast/base/ASTAllocator.h:20
    std::vector<ASTAny*> ptr_storage;                       // :97
    std::vector<ASTCleanupFunction> cleanup_fns;            // :105
public:
    template<typename T> FORCE_INLINE T* allocate() {       // :46
        static_assert(std::is_base_of<ASTAny, T>::value);
        return (T*) (void*) allocate_size(sizeof(T), alignof(T));
    }
    char* allocate_size(size_t obj_size, size_t alignment); // :56
    char* allocate_with_cleanup(size_t, size_t, void* fn);  // :64
    void clear();                                           // :84
    ~ASTAllocator();                                        // :89
};
```

`allocate<T>()` placement-news the object in arena memory and records the pointer in
`ptr_storage`. `object_heap_pointer()` computes an aligned offset, bumps `heap_offset`, and
allocates a new block when full (`ast/base/ASTAllocator.cpp:128-151`); a single object
larger than `heap_batch_size` grows the batch (`:140-143`). Allocation is guarded by the
shared `allocator_mutex` (`ast/base/ASTAllocator.cpp:154`), so a single allocator can be
used from multiple threads.

### Lifetime and destruction

The two allocator entry points differ in whether they track objects:

| Entry point | Tracked? | On `clear()` / destructor |
|-------------|----------|---------------------------|
| `ASTAllocator::allocate<T>()` / `allocate_size()` | Yes, via `store_ptr` (`ast/base/ASTAllocator.h:69`) | Calls `ptr->~ASTAny()` virtually, then frees blocks (`ast/base/ASTAllocator.cpp:96-101`) |
| `ASTAllocator::allocate_with_cleanup()` | Yes, via custom `cleanup_fn` (`ast/base/ASTAllocator.cpp:160`) | Calls the registered cleanup |
| `BatchAllocator::allocate_released<T>()` / `allocate_str()` | No | Memory only; no destructor |

`clear()` (`ast/base/ASTAllocator.cpp:63`) first destructs tracked pointers and runs cleanup
functions, then deletes every heap block except the first and resets the offset so the
allocator is immediately reusable. This is the per-pass reset mechanism.

> **Correction to a common assumption:** AST objects are **not** POD. Because the compiler
> supports `@delete` destructors on Chemical-defined types and AST nodes hold C++ members,
> `ASTAllocator` calls virtual destructors on `ptr_storage` (`ast/base/ASTAllocator.cpp:96`).
> Only the raw `BatchAllocator` paths (strings, `allocate_released`) skip destruction. Do not
> assume raw `memcpy`/`memset` of an `ASTAllocator`-tracked node is safe.

### Lifetime tiers

`LabBuildCompiler` owns several allocators with distinct lifetimes
(`compiler/lab/LabBuildCompiler.h:140-158`):

| Allocator | Lifetime | Purpose |
|-----------|----------|---------|
| `global_allocator` | Whole compiler session | Shared types built once in `TypeBuilder`, `nullValue`, `unresolvedDecl` |
| `job_allocator` | One build job | Job-scoped AST |
| `mod_allocator` | One module | Module-level declarations |
| `file_allocator` | One file | Per-file temporary nodes; cleared/reused between files |

The parser receives `global_allocator` and `mod_allocator` plus the `TypeBuilder`
(`parser/Parser.h:391-401`, constructor `:416-427`), and every parse helper takes an
`ASTAllocator&` argument (see `parser/structures/Block.cpp:11` and throughout `parser/`).
The interpreter uses a lightweight `InterpretScope::allocate<T>()` that routes to its own
`ASTAllocator` (`ast/base/InterpretScope.h:176`).

### Parallel parsing implications

- Each parallel unit gets its **own** allocator + `TypeBuilder`-adjacent state; the shared
  `global_allocator`/`type_builder` are used only for immortal flyweight types
  (`LabBuildCompiler.h:136-145`). This is why `GlobalBaseType::is_same` compares pointers
  (`ast/base/BaseType.h:786-791`) and `copy` returns `this` (`:793-799`).
- Allocation is mutex-guarded (`ast/base/ASTAllocator.cpp:154`), so sharing an allocator
  across threads is safe but serializes on the hot path — prefer per-file allocators.
- Because there is no per-node `delete`, freeing is O(blocks), not O(nodes); `clear()`
  reclaims a whole file in one shot between phases (`ast/base/ASTAllocator.cpp:63`).
- Never hold a raw `ASTAny*` past the lifetime of the allocator that owns it (e.g. across a
  `file_allocator.clear()`).

## `TypeBuilder` and Canonicalization

`TypeBuilder` (`ast/base/TypeBuilder.h:13`) is a flyweight cache: it allocates **one**
instance of each primitive type on the given allocator in `initialize()`
(`ast/base/TypeBuilder.cpp:27-60`) and hands out stable pointers via `getI8Type()`,
`getIntType()`, `getPointerType`, etc. It also owns shared singletons:

- all C/Chemical integer types (`i8`…`i128`, `u8`…`u128`, `char`, `short`, `int`, `long`,
  `u8`–`u64`) as value members (`ast/base/TypeBuilder.h:17-41`), including a
  `getIntNType(bitWidth, isUnsigned)` lookup (`:135`) and a `getIntNType(IntNTypeKind)`
  lookup (`:154`);
- `anyType`, `boolType`, `doubleType`, `floatType`, `longDoubleType`, `stringType`,
  `voidType`, `nullPtrType`, `ptrToVoid`, `ptrToAny`, `constPtrToAny`, `refToVoid`,
  `refToAny`, `expr_str_type` (`:44-58`);
- runtime/maybe-runtime wrappers (`:61-68`);
- `unresolvedDecl` — the placeholder linked to unresolved symbols (`:72`);
- `nullValue` — the shared `NullValue` singleton (`:74`).

`voidType` is special: it is a process-wide `static const VoidType` returned by
`getVoidTypeInstance()` (`ast/base/TypeBuilder.cpp:21-25`), not allocated.

### `canonical()` — alias/wrapper stripping

`BaseType::canonical()` (`ast/base/BaseType.cpp:170-201`) strips wrappers **one level deep**
(despite the `pure_type` alias at `ast/base/BaseType.h:120`):

```cpp
BaseType* BaseType::canonical() {
    switch(kind()) {
        case BaseTypeKind::Literal:     return as_literal_type_unsafe()->underlying;
        case BaseTypeKind::MaybeRuntime:return as_maybe_runtime_type_unsafe()->underlying;
        case BaseTypeKind::Runtime:     return as_runtime_type_unsafe()->underlying;
        case BaseTypeKind::Linked: {
            const auto linked = as_linked_type_unsafe()->linked;
            if(linked) {
                const auto known = linked->known_type();
                return known ? known != this ? known->canonical() : known : this;
            }
            return this;
        }
        case BaseTypeKind::Generic: {
            const auto gen = as_generic_type_unsafe();
            const auto can = gen->referenced->canonical();
            if(can->kind() == BaseTypeKind::Linked
               && can->as_linked_type_unsafe()->linked == gen->referenced->linked) return this;
            return can;
        }
        default: return this;
    }
}
```

Key points:

- A `LinkedType` canonicalizes to the `known_type()` of its linked node. A pointer to a
  type alias stays a pointer to the alias — `canonical()` is shallow, as the comment at
  `ast/base/BaseType.h:104-108` warns.
- `canonicalize_enum()` additionally maps an enum to its underlying integer
  (`ast/base/BaseType.cpp:203`).
- `is_same` / `satisfies` are used for compatibility (`ast/base/BaseType.h:182`/`:187`);
  `satisfies(Value*, bool)` canonicalizes the value's type (`ast/base/BaseType.cpp:724`).
- `GlobalBaseType` (used for the flyweights) overrides both `is_same` and `copy` to rely on
  pointer identity (`ast/base/BaseType.h:781-800`).

Type mutation and canonical views are also reached through helpers such as
`isPointerCanonical()`, `isReferenceCanonical()`, `getReferenceCanonical()`
(`ast/base/BaseType.cpp:607-672`), and the linked-node accessor family
`get_direct_linked_node()` / `get_linked_node()` / `get_linked_canonical_node()`
(`ast/base/BaseType.cpp:255-315`).

## `linked` and `ChildResolution` (Cross-References)

The parser produces unresolved names; symbol resolution wires them to declarations by
filling `linked` pointers. Because `BaseType` has no `TypeLoc`, the link for types lives in
`LinkedType::linked` (`ast/types/LinkedType.h:22`) and `GenericType::referenced`
(`ast/types/GenericType.h:18`); for identifiers it is `VariableIdentifier::linked`
(`ast/values/VariableIdentifier.h:30`).

`ASTAny::get_ref_linked_node()` (`ast/base/ASTAny.cpp:47`) is the uniform entry point: it
dispatches to `Value::linked_node()` or `BaseType::linked_node()` (both virtual,
`ast/base/Value.h:198`, `ast/base/BaseType.h:163`). `Value::linked_node()` returns the node
that resolves the next access-chain hop.

Child lookup is centralized in `ASTNode::child(resolver, name)`
(`ast/base/ASTNode.h:284`) and the free helpers declared in `ast/base/ChildResolution.h:11`:

```cpp
ASTNode* provide_child(const ChildResolver* resolver, Value* parent,
                       const chem::string_view& name, ASTNode* type_parent);   // :11
ASTNode* provide_child(const ChildResolver* resolver, BaseType* type,
                       const chem::string_view& name, ASTNode* type_parent);   // :16
```

`provide_child` unwraps runtime/reference/pointer wrappers on the type and consults the
type's linked container; passing a `ChildResolver*` (vs `nullptr`) additionally considers
members added to types via `impl` blocks. The default overload
`ASTNode::child(name)` calls with `nullptr` (`ast/base/ASTNode.h:293`).

Other cross-reference / lookup helpers on `ASTNode`:

| Helper | File | Purpose |
|--------|------|---------|
| `get_ancestor_by_kind(k)` | `ast/base/ASTNode.cpp:393` | Walk parents to a kind (uses `parent()`) |
| `root_parent()` | `ast/base/ASTNode.cpp:381` | Top-most node |
| `get_mod_scope()` / `get_file_scope()` | `ast/base/ASTNode.h:189-198` | Ancestor scope shortcuts |
| `get_members_container()` / `get_master_members_container()` | `ast/base/ASTNode.cpp:293` / `:308` | Resolve container through aliases/generics |
| `get_node_identifier()` | `ast/base/ASTNode.cpp:335` | Best-effort name of a node |
| `is_top_level()` / `isNonLocalDeclaration()` | `ast/base/ASTNode.cpp:162` / `:184` | Scope classification |

For the full declaration/linking pipeline see the
[Symbol Resolution skill](../symres/SKILL.md); for move-semantics tracking see the
[Interpreter skill](../interpreter/SKILL.md).

## Member Containers

Structs, unions, variants, interfaces, and impls share one container hierarchy so that
member lookup, constructors, destructors, inheritance, and vtables are implemented once.

```
ASTNode
└── VariablesContainer            (ast/structures/VariablesContainer.h)
    └── MembersContainer          (ast/structures/MembersContainer.h:39)
        ├── ExtendableMembersContainerNode   (ast/base/ExtendableMembersContainerNode.h:8)
        │   ├── StructDefinition  (ast/structures/StructDefinition.h:79)
        │   ├── UnionDef
        │   ├── VariantDefinition (ast/structures/VariantDefinition.h:29)
        │   └── InterfaceDefinition
        └── ImplDefinition
```

- `MembersContainer` holds the evaluated node list (`evaluated_container`,
  `ast/structures/MembersContainer.h:46`), constructs/destructors, inherited-type metadata,
  and the automatic-function generation pass (`generate_automatic_functions`,
  `ast/structures/MembersContainer.h:271`). Iteration is exposed through filtered ranges
  (`instantiations`, `master_functions`, `non_gen_range` — `:112-137`).
- `ExtendableMembersContainerNode` adds the container `identifier` and an
  `extension_functions` list for extension methods added outside the definition
  (`ast/base/ExtendableMembersContainerNode.h:16-22`, `add_extension_func` at `:58`).
  Its `as_extendable_members_container_unsafe()` accepts struct/union/variant/interface but
  **not** impl (`ast/base/ASTNode.h:972`).
- Members are `BaseDefMember`s (`ast/structures/BaseDefMember.h:8`) — the storage-backed
  fields. `StructMember` (`ast/structures/StructMember.h:31`) carries a `TypeLoc type`, a
  default `Value*`, and `StructMemberAttributes` (access, const, alignment, volatile).
  Variant members (`VariantMember`), unnamed structs/unions, and variant member params are
  also covered by `isBaseDefMember` / `isStoredStructType` (`ast/base/ASTNode.h:589`,
  `:593`).
- Generic containers share a "master" container: `get_master_members_container()`
  (`ast/base/ASTNode.cpp:308`) unwraps `Generic*Decl` to its `master_impl`, and generic
  instantiations are separate `MembersContainer`s linked back via `generic_parent` /
  `generic_instantiation` (`ast/structures/MembersContainer.h:53-58`). See the
  [Generics skill](../generics/SKILL.md).

## Gotchas

1. **`as_*_unsafe()` is only checked in debug builds.** `CHECK_CAST`/`CHECK_COND` are
   compiled out in release (`ast/base/DebugCast.h:13-19`). A wrong cast corrupts memory
   silently in release; reproduce with a debug build to get the `abort()`.
2. **New node kinds silently fall through visitors in release.** The `default:` throws only
   under `#ifdef DEBUG` (`preprocess/visitors/NonRecursiveVisitor.h:728`). Add handling to every
   relevant visitor or release builds will misinterpret the node.
3. **Never insert enum values in the middle.** `ASTNodeKind`/`ValueKind`/`BaseTypeKind` are
   mirrored 1:1 in `lang/libs/compiler/src/ast/base/*.ch`. Inserting shifts every later
   value and corrupts TCC-compiled plugins (see the CRITICAL enum-sync rule in
   `AGENTS.md`). Append only.
4. **`Value` and `BaseType` have no `_parent`.** Only `ASTNode` participates in the parent
   chain. When you have a `Value`, get to the tree via `linked_node()` or the containing
   `ASTNode`.
5. **`TypeLoc` is const.** Fixing a type means writing back through the owning node (e.g.
   `setType`) as `RecursiveVisitor::VisitCastedValue` does
   (`preprocess/visitors/RecursiveVisitor.h:202-204`).
6. **`canonical()` is shallow.** It strips one alias/wrapper level; nested aliases and
   pointers require repeated calls or the `*Canonical()` helpers
   (`ast/base/BaseType.h:104`).
7. **Every copy needs an allocator.** `copy(ASTAllocator&)` is pure virtual on `BaseType`
   (`ast/base/BaseType.h:128`) and overridden throughout; deep copies are expensive and
   only appropriate before parallel passes split work.
8. **`WhereClause` is an `ASTAny` but not an `ASTNode`** (`ast/structures/WhereClause.h:40`).
   Do not pass it to `visit(ASTNode*)` or assume `kind()` exists.
9. **Do not mix allocation lifetimes.** A `Value*` allocated on `file_allocator` must not
   survive past that file's processing; move data onto `mod_allocator`/`global_allocator`
   if it must outlive the file.

## Extension Checklist: Adding a New AST Node

1. **Define the class.** Add `ast/<category>/YourNode.h` (and `.cpp` if it has behavior).
   Derive from `ASTNode`, `Value`, or `BaseType` as appropriate, and pass the correct kind
   to the base constructor. For a value, set `ValueKind`; for a type, `BaseTypeKind`.
2. **Add the kind.** Append `YourNode` to the end of the relevant enum in
   `ast/base/ASTNodeKind.h` / `ValueKind.h` / `BaseTypeKind.h`. **Append, never insert.**
3. **Expose it in `ast_fwd.h`.** Add a forward declaration in `ast/base/ast_fwd.h` (keep the
   section: Nodes / Values Begins / Types).
4. **Add range predicates and casts.** In `ASTNode.h` add `isYourNode(ASTNodeKind)` and, if
   it belongs to a category, extend the relevant multi-kind helper
   (`isBaseDefMember`, `isMembersContainer`, …). Add `as_your_node()` (nullable) and
   `as_your_node_unsafe()` (`CHECK_CAST`). Mirror this on `Value`/`BaseType` if applicable.
5. **Sync the CBI binding.** If the kind is exposed to plugins, append the same value to
   `lang/libs/compiler/src/ast/base/ASTNodeKind.ch` (or `ValueKind.ch` /
   `BaseTypeKind.ch`). A mismatch causes SIGSEGV in TCC-compiled plugins.
6. **Add visitor handling.** Add a `VisitYourNode` default to
   `preprocess/visitors/NonRecursiveVisitor.h` (forwarding to `VisitCommonNode`/`Value`/
   `Type`) and a `case` in the matching `*NoNullCheck` switch. If the node has children,
   add a `VisitYourNode` in `preprocess/visitors/RecursiveVisitor.h` that calls `visit_it`
   on each child.
7. **Handle codegen.** For LLVM: override `code_gen`/`code_gen_declare`/`add_child_index`/
   `llvm_load` on `ASTNode` or `llvm_value`/`llvm_pointer`/`llvm_type` on `Value`/`BaseType`
   (all under `#ifdef COMPILER_BUILD`). For C: handle it in `ToCAstVisitor`
   (`preprocess/2c/2cASTVisitor.cpp`) and add a forward declaration in
   `CTopLevelDeclarationVisitor` if needed.
8. **Handle the interpreter.** If the node can be evaluated at comptime, implement
   `evaluated_value(InterpretScope&)` on values and node interpretation in
   `compiler/Interpreter/Core.cpp` (respecting move semantics — see the
   [Interpreter skill](../interpreter/SKILL.md)).
9. **Handle type checking / symres.** Add linking logic in the appropriate symres pass and
   verification in `compiler/typeverify/TypeVerify.*`.
10. **Handle the parser.** Parse the construct into the new node in `parser/` and pass the
    correct `ASTAllocator&`.
11. **Handle the binding API** if plugins should build/inspect the node: add wrappers in
    `lang/libs/compiler/src/` (see the [Compiler API skill](../compiler_api/SKILL.md)).
12. **Add tests.** Use `lang/tests/` and add compiler-failure negative tests in `lang/tests/negative/` (see the [Testing Guide skill](../testing/SKILL.md)).
## Related Skills

- [Compiler API](../compiler_api/SKILL.md) — binding-side AST access and cast inventory
- [Parser Internals](../parser_internals/SKILL.md) — how these nodes are constructed
- [Symbol Resolution](../symres/SKILL.md) — how `linked` / `ChildResolver` are populated
- [Generics](../generics/SKILL.md) — generic declarations, masters, and instantiation
- [Type Verification](../type_verification/SKILL.md) — the `TypeVerifier` recursive visitor
- [Interpreter Internals](../interpreter/SKILL.md) — `InterpretScope`, move semantics, temp destruction
- [LLVM Backend](../llvm_backend/SKILL.md) — node virtual-method codegen
- [C Codegen (2c)](../c_codegen/SKILL.md) — `ToCAstVisitor` traversal
- [Performance](../performance/SKILL.md) — arena allocation and parallelization
- [CBI Plugin API](../cbi_plugin_api/SKILL.md) — the enum-sync contract with plugins
