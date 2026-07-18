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

## Visitor Pattern — AST Traversal Framework

The compiler has a three-tier visitor hierarchy that all backends and passes build upon. Understanding this hierarchy is essential for adding new compiler features, backends, or analysis passes.

### Tier 1: NonRecursiveVisitor (Base Dispatch)

**File:** `preprocess/visitors/NonRecursiveVisitor.h`

This is the CRTP (Curiously Recurring Template Pattern) base class that all visitors inherit from. It provides:

```cpp
template<typename Derived>
class NonRecursiveVisitor {
public:
    // Three "common" hooks that fire for ALL nodes/values/types:
    inline void VisitCommonNode(ASTNode* node);    // Override to intercept ALL ASTNode visits
    inline void VisitCommonValue(Value* value);    // Override to intercept ALL Value visits
    inline void VisitCommonType(BaseType* type);   // Override to intercept ALL BaseType visits

    // Node dispatch — VisitNodeNoNullCheck() switches on ASTNodeKind:
    void VisitNodeNoNullCheck(ASTNode* node) {
        switch(node->kind()) {
            case ASTNodeKind::AssignmentStmt:
                static_cast<Derived*>(this)->VisitAssignmentStmt((AssignStatement*) node);
                return;
            case ASTNodeKind::FunctionDecl:
                static_cast<Derived*>(this)->VisitFunctionDecl((FunctionDeclaration*) node);
                return;
            // ... 60+ more cases
        }
    }

    // Value dispatch — VisitValueNoNullCheck() switches on ValueKind:
    void VisitValueNoNullCheck(Value* value) {
        switch(value->kind()) {
            case ValueKind::FunctionCall:
                static_cast<Derived*>(this)->VisitFunctionCall((FunctionCall*) value);
                return;
            case ValueKind::StructValue:
                static_cast<Derived*>(this)->VisitStructValue((StructValue*) value);
                return;
            // ... 40+ more cases
        }
    }

    // Type dispatch — VisitTypeNoNullCheck() switches on BaseTypeKind:
    void VisitTypeNoNullCheck(BaseType* type) {
        switch(type->kind()) {
            case BaseTypeKind::Pointer:
                static_cast<Derived*>(this)->VisitPointerType((PointerType*) type);
                return;
            // ... 20+ more cases
        }
    }

    // Default implementations — every node/value/type has a default no-op visit:
    inline void VisitAssignmentStmt(AssignStatement* node) {
        static_cast<Derived*>(this)->VisitCommonNode(node);
    }
    inline void VisitFunctionCall(FunctionCall* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }
    inline void VisitPointerType(PointerType* type) {
        static_cast<Derived*>(this)->VisitCommonType(type);
    }
    // ... and 60+ more
};
```

**Key design decisions:**

1. **CRTP** — No virtual dispatch overhead. `static_cast<Derived*>(this)->visit(...)` resolves at compile time.
2. **No null checks on internal paths** — `VisitNodeNoNullCheck()` skips null checks; `VisitNode()` wraps with a null guard. This lets callers choose performance vs. safety.
3. **Common fallbacks** — `VisitCommonNode()`, `VisitCommonValue()`, `VisitCommonType()` give derived classes a single override point to intercept all visits of a category.
4. **DEBUG-only throws** — In DEBUG builds, the `default:` case in each switch throws `CHEM_THROW_RUNTIME("UNHANDLED: node kind in non recursive visitor")`. This catches missing visitor methods early.
5. **Null safety** — `visit(ASTNode*)`, `visit(Value*)`, `visit(BaseType*)` all check for null before dispatching. Useful when optional children are present.

### Tier 2: RecursiveVisitor (Deep Traversal)

**File:** `preprocess/visitors/RecursiveVisitor.h`

Inherits from `NonRecursiveVisitor<Derived>` and adds **recursive traversal** for every node type. For each `Visit*()` method, it calls `visit_it()` on all child nodes:

```cpp
template<typename Derived>
class RecursiveVisitor : public NonRecursiveVisitor<Derived> {
public:
    // Helper — dispatches back to the Derived class:
    inline void visit_it(Scope& scope) {
        static_cast<Derived*>(this)->VisitScope(&scope);
    }
    inline void visit_it(Value* value) {
        static_cast<Derived*>(this)->visit(value);  // Goes through NonRecursiveVisitor dispatch
    }
    inline void visit_it(ASTNode* node) {
        static_cast<Derived*>(this)->visit(node);
    }
    template<typename T>
    inline void visit_it(BaseType*& type, SourceLocation location) {
        static_cast<Derived*>(this)->visit(type, location);
    }

    // Examples of recursive traversal:
    void VisitScope(Scope *scope) {
        for(auto& node : scope->nodes) visit_it(node);
    }
    void VisitFunctionCall(FunctionCall *call) {
        visit_it(call->parent_val);
        for(auto& arg : call->generic_list) visit_it(arg);
        for(auto& val : call->values) visit_it(val);
    }
    void VisitFunctionDecl(FunctionDeclaration *decl) {
        for(auto& param : decl->params) visit_it(param);
        visit_it(decl->returnType);
        if(decl->body.has_value()) visit_it(decl->body.value());
    }
    void VisitIfStmt(IfStatement *stmt) {
        visit_it(stmt->condition);
        visit_it(stmt->ifBody);
        for (auto& elif : stmt->elseIfs) {
            visit_it(elif.first); visit_it(elif.second);
        }
        if(stmt->elseBody.has_value()) visit_it(stmt->elseBody.value());
    }
    void VisitStructDecl(StructDefinition *def) {
        VisitVariables(def->variables());
        for(const auto func : def->evaluated_nodes()) {
            NonRecursiveVisitor<Derived>::visit(func);
        }
    }
    void VisitGenericStructDecl(GenericStructDecl* node) {
        for(const auto decl : node->instantiations) {
            static_cast<Derived*>(this)->VisitStructDecl(decl);
        }
    }
    void VisitImplDecl(ImplDefinition* def) {
        visit_it(def->interface_type);
        visit_it(def->struct_type);
        for(const auto func : def->evaluated_nodes()) {
            NonRecursiveVisitor<Derived>::visit(func);
        }
    }
    void VisitLambdaFunction(LambdaFunction *func) {
        for(auto& var : func->captureList) visit_it(var);
        visit_it(func->scope);
    }
    void VisitArrayType(ArrayType *type) {
        visit_it(type->elem_type);
        if(type->array_size_value) visit_it(type->array_size_value);
    }
    // ... and 50+ more recursive methods covering ALL AST node types
};
```

**Key patterns:**
- `visit_it()` is the bridge — it calls back to `Derived::visit()` which goes through `NonRecursiveVisitor`'s `VisitNodeNoNullCheck()` / `VisitValueNoNullCheck()` switch dispatch. This means a derived visitor only needs to override the `Visit*()` methods it cares about — `RecursiveVisitor` handles the recursion.
- **Generic instantiations** are traversed via their concrete instantiation methods: `VisitGenericStructDecl` iterates `node->instantiations` and calls `VisitStructDecl` for each.
- **Variables in structs/interfaces/variants** are traversed via `VisitVariables()` which iterates `BaseDefMember*` list.

### Tier 3: Concrete Visitors

#### a) RepresentationVisitor (Code → Text)

**Files:** `preprocess/RepresentationVisitor.h`, `preprocess/RepresentationVisitor.cpp`

Converts AST nodes back to Chemical source code text. Inherits from `NonRecursiveVisitor<RepresentationVisitor>` directly (no recursion — it visits the root and controls output itself).

```cpp
class RepresentationVisitor : public NonRecursiveVisitor<RepresentationVisitor> {
public:
    std::ostream& output;              // Output stream
    unsigned int indentation_level = 0; // Current indent
    bool interpret_representation = false; // If true, no quotes on strings (for interpreter)
    bool nested_value = false;          // If true, no semicolons on function calls

    // Output methods:
    void write(char value);
    void indent();
    void new_line_and_indent();
    void write_str(const std::string& value);

    // Main entry:
    void translate(std::vector<ASTNode*>& nodes);  // Visit a list of top-level nodes

    // Node visitors — each Visit*() writes the Chemical source for that construct:
    void VisitVarInitStmt(VarInitStatement *init);  // "var x : int = 5"
    void VisitFunctionDecl(FunctionDeclaration *decl);  // "func foo() { ... }"
    void VisitStructDecl(StructDefinition *def);  // "struct Foo { ... }"
    void VisitFunctionCall(FunctionCall *call);   // "foo()"
    void VisitExpression(Expression *expr);       // "(a + b)"
    // ... and many more

    // Common throw — ensures every node type is handled:
    void VisitCommonNode(ASTNode* node) {
        CHEM_THROW_RUNTIME("RepresentationVisitor::VisitCommonNode called");
    }
};
```

**Used by:** Error messages (showing code snippets), interpreter (expressive string printing with `interpret_representation = true`), debug output.

#### b) ToCAstVisitor (C Codegen)

**Files:** `preprocess/2c/2cASTVisitor.h`, `preprocess/2c/2cASTVisitor.cpp` (~3000 lines)

The C translation backend. Inherits from both `NonRecursiveVisitor<ToCAstVisitor>` and `ASTDiagnoser`.

```cpp
class ToCAstVisitor : public NonRecursiveVisitor<ToCAstVisitor>, public ASTDiagnoser {
public:
    BufferedWriter writer;             // Large in-memory buffer for output
    CTopLevelDeclarationVisitor tld;    // Forward-declares structs/functions
    CDestructionVisitor destructor;     // Manages destructor calls
    NameMangler& mangler;              // Name mangling
    bool pass_structs_to_initialize;   // Sret pattern
    bool is64Bit;                      // Platform bitness
    bool minify;                       // Remove whitespace
    // ... and many more state fields

    // Lifecycle:
    void prepare_translate();
    void end_translate();              // Generate stub impls for static interfaces
    void file_level_reset();           // Clear per-file state
    void reset();                      // Reuse for next module
    void declare_and_translate(const std::vector<ASTNode*>& nodes); // Two-pass: declare then implement

    // Forward declarations:
    void fwd_declare(ASTNode* node);                   // Emit function/variable forward decl
    void declare_type_alias(ASTNode* node);            // Emit typedef
    void fwd_declare(BaseType* type);                  // Resolve type to C decl

    // All Visit*() methods emit C code:
    void VisitVarInitStmt(VarInitStatement* node);     // "Type var = ..."
    void VisitFunctionDecl(FunctionDeclaration* node); // "Type func(params) { body }"
    void VisitStructDecl(StructDefinition* node);      // "struct Name { ... }"
    void VisitIfStmt(IfStatement* node);               // "if(cond) { ... }"
    void VisitFunctionCall(FunctionCall* value);        // "func(args)"
    // ... 60+ more

    // Value-as-statement helpers:
    void writeReturnStmtFor(Value* value);
    void writeIfStmtValue(IfStatement& value);
    void writeSwitchStmtValue(SwitchStatement& value, BaseType* type);
    void writeLoopStmtValue(LoopBlock& value, BaseType* type);
};
```

**See related:** [C Codegen Skill](.agents/skills/c_codegen/SKILL.md) for detailed ToCAstVisitor documentation.

#### c) CDestructionVisitor (Destructor Management)

**Files:** `preprocess/2c/CDestructionVisitor.h`, `preprocess/2c/CDestructionVisitor.cpp`

Manages destructor calls in C codegen. Works with `ToCAstVisitor` to emit cleanup code when scopes end, returns happen, or statements throw:

```cpp
class CDestructionVisitor : public SubVisitor {
public:
    std::vector<DestructionJob> destruct_jobs;  // Pending destructor calls

    void destruct(const chem::string_view& self_name, MembersContainer* linked,
                  FunctionDeclaration* destructor, bool is_pointer);
    void queue_destruct(std::string self_name, ASTNode* initializer,
                        MembersContainer* linked, bool is_pointer, bool has_drop_flag);
    void queue_destruct_arr(std::string self_name, ASTNode* initializer,
                            BaseType* elem_type, int array_size);
    void dispatch_jobs_from(int begin);  // Emit all destructors from job index
};
```

#### d) SubVisitor (Base for C Codegen Sub-Visitors)

**File:** `preprocess/2c/SubVisitor.h`

Minimal base class for sub-visitors that operate within `ToCAstVisitor`:

```cpp
class SubVisitor {
public:
    ToCAstVisitor& visitor;
    SubVisitor(ToCAstVisitor& visitor) : visitor(visitor) {}
    inline void space() const { visitor.space(); }
    inline void write(char value) const { visitor.write(value); }
    inline void write(const chem::string_view& value) const { visitor.write(value); }
    inline void new_line_and_indent() { visitor.new_line_and_indent(); }
};
```

**Used by:** `CTopLevelDeclarationVisitor`, `CDestructionVisitor`, `CValueDeclarationVisitor`

#### e) CTopLevelDeclarationVisitor (Forward Declarations)

**File:** `preprocess/2c/CTopLevelDeclVisitor.h`

Handles C forward declarations. Called BEFORE `translate_after_declaration()` to emit function prototypes, struct declarations, and typedefs so the C compiler doesn't complain about unknown types/functions.

### Visitor Usage Summary

| Visitor | Purpose | Base | Recursive? |
|---------|---------|------|-----------|
| `NonRecursiveVisitor<D>` | Base dispatch layer | — | No (base) |
| `RecursiveVisitor<D>` | Deep AST traversal | NonRecursiveVisitor | Yes (via visit_it) |
| `RepresentationVisitor` | AST → Chemical text | NonRecursiveVisitor | Manual |
| `ToCAstVisitor` | AST → C code | NonRecursiveVisitor + ASTDiagnoser | Manual |
| `TypeVerifier` | Type checking | NonRecursiveVisitor | Manual |
| `GenericInstantiationPass` | Generic monomorphization | NonRecursiveVisitor | Manual |
| `SymResLinkBody` | Body symbol resolution | NonRecursiveVisitor | Manual |

### How to Create a New Visitor

1. **Inherit from `NonRecursiveVisitor<YourVisitor>`** if you only need dispatch
2. **Inherit from `RecursiveVisitor<YourVisitor>`** if you need full tree walk
3. **Override specific `Visit*()` methods** for nodes you care about
4. **Override `VisitCommon*()`** if you want to intercept ALL nodes/values/types
5. **Call `visit(node)` or `visit(value)` or `visit(type)`** to dispatch to the right `Visit*()` method
6. **For deep traversal**, call `VisitScope(&scope)` or individual visit methods

Example skeleton:
```cpp
class MyAnalysisVisitor : public RecursiveVisitor<MyAnalysisVisitor> {
public:
    void VisitFunctionCall(FunctionCall* call) override {
        // Called for every function call in the AST
        count++;
        // Don't forget to recurse! RecursiveVisitor handles this
        RecursiveVisitor<MyAnalysisVisitor>::VisitFunctionCall(call);
    }
    int count = 0;
};

MyAnalysisVisitor visitor;
visitor.visit(someScope);  // Traverses entire scope tree
std::cout << "Found " << visitor.count << " function calls";
```

**Warning:** When using `RecursiveVisitor`, you MUST call the base class `Visit*()` method if you override it, or no recursion will happen. `RecursiveVisitor::VisitFunctionCall()` calls `visit_it()` on children — if you override without calling `super`, children won't be traversed.

## Source Location System

The source location system is how the Compiler tracks where every token, node, and type came from in the source code. It's a compact, thread-safe uint64 encoding used throughout the compiler.

### SourceLocation

**File:** `core/source/SourceLocation.h`

```cpp
class SourceLocation {
public:
    uint64_t encoded;  // 64-bit encoded location

    constexpr SourceLocation(uint64_t encoded) : encoded(encoded) {}

    inline bool isValid() const { return encoded != 0; }
    inline bool isInvalid() const { return encoded == 0; }
};

// Zero means "no location" — used for synthetic/generated nodes
inline constexpr uint64_t ZERO_LOC = 0;
```

A `SourceLocation` is just a `uint64_t`. Zero is the "invalid" location. Every `ASTNode` and `Value` has an `encoded_location()` getter that returns its `SourceLocation`.

### LocationManager

**File:** `core/source/LocationManager.h`, `core/source/LocationManager.cpp`

`LocationManager` is a thread-safe service that:
1. Encodes file paths into compact file IDs
2. Encodes line:column ranges into `uint64_t` values
3. Decodes locations back into human-readable form

#### Bit Layout (for small locations)

Most locations fit within 63 bits using this layout:

```
Bit:  63  62 ... 53  52 ... 35  34 ... 23  22 ... 12  11 ... 0
      ↓           ↓           ↓           ↓           ↓
    [not idx]  [file_id]  [line_start] [char_start] [line_end_offset] [char_end]
                 10 bits    18 bits      12 bits       11 bits          12 bits
```

- **File ID**: 10 bits → max 1024 files
- **Line Start**: 18 bits → max 262,143 lines
- **Char Start**: 12 bits → max 4095 columns
- **Line End Offset** (relative to line start): 11 bits → max 2047 lines difference
- **Char End**: 12 bits → max 4095 columns
- **Bit 63**: 0 means "small encoded", 1 means "index into large-locations vector"

#### Large Locations (overflow path)

When a location exceeds the bit constraints (e.g., line number > 262,143), it's stored in a `std::vector<LocationData>` and the `uint64_t` stores an index with bit 63 set:

```cpp
// Encoding:
if(fits_in_bits) {
    return bitpacked_location;  // Bit 63 = 0
} else {
    std::lock_guard guard(location_mutex);
    locations.push_back({fileId, lineStart, charStart, lineEnd, charEnd});
    return INDICATOR_BIT_MASK | locations.size() - 1;  // Bit 63 = 1
}
```

#### Key API

```cpp
class LocationManager {
public:
    // File tracking — thread-safe (mutex-protected)
    unsigned int encodeFile(const std::string& filePath);
    int encodeExistingFile(const std::string& filePath);
    std::string_view getPathForFileId(unsigned int fileId);

    // Location encoding — thread-safe for large locations, lock-free for small
    uint64_t addLocation(uint32_t fileId, uint32_t lineStart,
                         uint32_t charStart, uint32_t lineEnd, uint32_t charEnd);

    // Location decoding
    LocationData getLocation(uint64_t data) const;
    LocationPosData getLocationPos(SourceLocation loc) const;
    uint32_t getLineStartFast(SourceLocation loc);  // Optimized: extracts line without full decode

    // Formatting
    std::string formatLocation(SourceLocation location);  // "path/file.ch:42:10:"
};
```

#### LocationData Structure

```cpp
struct LocationData {
    uint32_t fileId;
    uint32_t lineStart;
    uint32_t charStart;
    uint32_t lineEnd;
    uint32_t charEnd;
};
```

#### Thread Safety

- **`file_mutex`** (`std::mutex`): Guards file path registration — multiple threads may call `encodeFile()` concurrently
- **`location_mutex`** (`std::mutex`): Guards the large-locations vector — only used when locations exceed bit limits
- **Small location encoding is lock-free**: The bit-packing math is purely arithmetic, so no synchronization needed

#### Usage in AST

Every `ASTNode` and `Value` stores its location:
```cpp
class ASTNode : public ASTAny {
    SourceLocation encoded_location;
public:
    const SourceLocation& encoded_location() const { return encoded_location; }
};

class Value : public ASTAny {
    SourceLocation encoded_location;
public:
    SourceLocation encoded_location() const { return encoded_location; }
};
```

During parsing, the lexer tracks character position and the parser creates `SourceLocation` values via `LocationManager::addLocation()`. During diagnostics, the `ASTDiagnoser` uses `LocationManager::formatLocation()` to produce:
```
error: path/to/file.ch:42:10: cannot assign to immutable variable
```

#### Related Position/File Structures

| Structure | File | Purpose |
|-----------|------|---------|
| `struct Position` | `core/diag/Position.h` | Line and character position within a file |
| `struct Range` | `core/diag/Range.h` | Start and end Position forming a span |
| `struct Location` | `core/diag/Location.h` | File path + Range — high-level location representation |
| `Diag` | `core/diag/Diagnostic.h` | Diagnostic message with Location for error/warning reporting |
| `StreamPosition` | `stream/StreamPosition.h` | Lexer position tracking (data ptr, line index, char index) |
| `InputSource` | `stream/InputSource.h` | Abstract base for source input |
| `FileInputSource` | `stream/FileInputSource.h` | Concrete file-backed input source |
| `SourceProvider` | `stream/SourceProvider.h` | Reads from InputSource, tracks line/char numbers |

### The CHECK_COND Debug Macro

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
- **Debug builds**: Every unsafe cast checks the discriminator — catches bugs early. The `NonRecursiveVisitor` `default:` case also throws in DEBUG, catching unhandled node kinds.
- **Release builds**: Just a static_cast — maximum performance. The `default:` case is unreachable.
- If you see a "FATAL" message in CI, it means an `as_*_unsafe()` cast was used on the wrong type, or a visitor method was not overridden for a new node kind.

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
