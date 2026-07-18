---
name: Symbol Resolution (symres)
description: Comprehensive guide to the Chemical compiler's symbol resolution pipeline — how symbols are declared, linked, and resolved across files, modules, and generic instantiations. Critical for stabilizing and optimizing the symres module.
---

# Symbol Resolution Pipeline

The symbol resolution pipeline is the semantic analysis backbone of the Chemical compiler. It runs after parsing and before type verification, transforming a flat AST into a fully linked symbol graph.

## Architecture Overview

### Pipeline Phases (per module)

```
1. Import Resolution        — ImportPathHandler resolves module dependencies
2. Top-Level Declaration    — TopLevelDeclSymDeclare visits all files, declares top-level symbols
3. Link Signatures          — TopLevelLinkSignature resolves type signatures (no bodies)
4. Generic Instantiation    — GenericInstantiationPass creates concrete instantiations
5. Link Bodies              — SymResLinkBody resolves function bodies, statements, and expressions
6. Type Verify              — type_verify() validates types (separate pass, in compiler/typeverify/)
```

### Key Files

| File | Purpose |
|------|---------|
| `compiler/symres/SymbolResolver.h` | Core `SymbolResolver` class — owns `SymbolTable`, `ChildResolver`, manages the per-phase API |
| `compiler/symres/SymbolResolver.cpp` | Implementation — `tld_declare_file()`, `declare_and_link_file()`, `import_file()` |
| `compiler/symres/DeclareTopLevel.h` | `TopLevelDeclSymDeclare` visitor — declares symbols without linking types |
| `compiler/symres/LinkSignature.h` | `TopLevelLinkSignature` visitor — links type signatures only (no bodies) |
| `compiler/symres/SymResLinkBody.h` | `SymResLinkBody` visitor — links function bodies, statements, expressions |
| `compiler/symres/GenericInstantiationPass.h` | `GenericInstantiationPass` — creates generic instantiations after link signature |
| `compiler/symres/SymbolTable.h` | `SymbolTable` class — stores and resolves symbols with scope nesting |
| `compiler/symres/ImplementationsIndex.h` | `ImplementationsIndex` — indexes all `impl` blocks for operator overload resolution |
| `compiler/symres/CoreNodes.h` | `CoreNodes` — provides core interfaces (ops::Add, iterable::Iterable, etc.) |
| `compiler/symres/ChildResolver.h` | `ChildResolver` — resolves `impl`-provided methods on types |

## The SymbolResolver Class

`SymbolResolver` is the central orchestrator. It inherits from `ASTDiagnoser` for error reporting.

### Key Owned Objects

| Member | Type | Purpose |
|--------|------|---------|
| `table` | `SymbolTable` | The main symbol table — all module symbols live here |
| `child_resolver` | `ChildResolver` | Resolves extension methods from `impl` blocks |
| `comptime_scope` | `GlobalInterpretScope&` | Needed for comptime condition resolution |
| `implsIndex` | `ImplementationsIndex&` | Operator overload resolution via indexed impl blocks |
| `genericInstantiator` | `GenericInstantiatorAPI` | Creates concrete generic instantiations |
| `instContainer` | `InstantiationsContainer&` | Tracks all generic instantiations |
| `generic_inst_reg_mutex` | `std::recursive_mutex` | Thread-safe generic instantiation registration |

### Symbol Resolution Files (File Level)

A module consists of multiple files. Each file goes through the declaration + linking process:

```cpp
// In SymbolResolver.cpp:
SymbolRange SymbolResolver::tld_declare_file(Scope& scope, unsigned int fileId, const std::string& abs_path) {
    // 1. Create a TopLevelDeclSymDeclare visitor
    // 2. Visit all top-level nodes in the file
    // 3. Return SymbolRange (start and end indices of file-private symbols)
}

void SymbolResolver::declare_and_link_file(Scope& scope, unsigned int fileId, const std::string& abs_path) {
    // 1. Call tld_declare_file() to declare all symbols
    // 2. Create a TopLevelLinkSignature visitor — link type signatures
    // 3. Create a SymResLinkBody visitor — link function bodies
    // 4. Merge diagnostics back
}
```

### Parallelization

Files within a module are symbol-resolved **in parallel**. Each file gets its own:

- `TopLevelLinkSignature` — owns its own `SymbolTable` for file-private symbols
- `SymResLinkBody` — owns its own `SymbolTable`, `GenericInstantiatorAPI`, and `ASTDiagnoser`
- Diagnostics are collected per-file and merged back after linking

This design allows N files in a module to be processed concurrently without mutating the shared `SymbolResolver` state except during well-defined declaration phases.

#### Thread Safety Mechanism

1. **Declaration phase (serial)**: `tld_declare_file()` runs serially — only one file declares symbols at a time
2. **Signature phase (parallel)**: Each file gets its own `TopLevelLinkSignature` with its own `SymbolTable`. The shared resolver is only queried (read-only)
3. **Body phase (parallel)**: Each file gets its own `SymResLinkBody` with per-file `SymbolTable` and `GenericInstantiatorAPI`

The shared state that IS mutated:
- `generic_inst_reg_mutex` — protects generic instantiation registration maps
- `implsIndex.index_mutex` — a `std::shared_mutex` (shared_lock for reads, unique_lock for writes)
- `diagnostics` — merged back after each phase completes

## Phase 1: Top-Level Declaration

### TopLevelDeclSymDeclare

This visitor walks top-level declarations and registers them in the symbol table:

| Node Type | Declaration Behavior |
|-----------|---------------------|
| `FunctionDeclaration` | Declares with overload support — multiple functions with the same name in the same scope become `MultiFunctionNode` |
| `StructDefinition` | Declares the struct name |
| `VariantDefinition` | Declares the variant name |
| `UnionDef` | Declares the union name |
| `InterfaceDefinition` | Declares the interface |
| `ImplDefinition` | Not declared directly — registered in `ImplementationsIndex` |
| `Namespace` | Opens a new scope, declares all children inside it |
| `GenericXxxDecl` | Declared as a generic — type checking happens later |
| `IfStatement` | If `comptime if`, evaluates the condition and only declares the winning branch |
| `VarInitStatement` | Top-level variables are declared |
| `TypealiasStatement` | Type aliases are declared |
| `ImportStatement` | Resolved by the ImportPathHandler, symbols imported into the table |

### Access Specifiers

```
public    — Exposed to other modules
private   — Only accessible in the current file (scope_end() removes them)
internal  — Only accessible in the current module (module_scope_end() removes them)
protected — Visible to inheriting structs only (for struct members)
```

The `SymbolTable` tracks scope nesting:
- **Global scope**: permanent symbols
- **Module scope**: symbols that die when the module is done
- **File scope**: symbols that die when the file is done (private declarations)
- **Default scope**: used for namespaces, struct members, etc.

## Phase 2: Link Signatures

### TopLevelLinkSignature

This visitor resolves **type signatures only** — never enters function bodies:

| What it links | What it skips |
|---------------|---------------|
| Function parameter types | Function body statements |
| Return types | If/while/for body scopes |
| Struct member types | Assignment expressions |
| Variant member types | Variable initializer values |
| Interface method signatures | Lambda bodies |
| Type alias definitions | - |
| Generic type parameters | - |

#### Key Design Decisions

1. **Signature resolution is separate from body resolution** because:
   - Generics need their signatures resolved first to determine whether to instantiate
   - Forward references between files require all signatures to be established before any body can use them
   - Errors in signatures should be caught before attempting body resolution

2. **Per-file SymbolTable**: `TopLevelLinkSignature.table` holds file-private symbols (generic type params, using-imports, aliases). These are dropped when the visitor is destroyed.

3. **Inline instantiations**: When a `GenericType` is encountered in a signature, an inline instantiation is requested. These are stored in `inline_instantiations` for processing after the signature pass.

## Phase 3: Generic Instantiation Pass

### GenericInstantiationPass

This pass runs BETWEEN link signatures and link bodies. It:

1. Finalizes inline instantiations from the link signature phase (calls `GenericTypeDecl::finalize_signature()`)
2. Visits all generic type declarations and ensures their instantiations are registered
3. Does NOT visit function bodies — only signatures

This separation allows the generic instantiation pass to use a parallel model where each instantiation can be finalized independently.

See [generics skill](./.agents/skills/generics/SKILL.md) for detailed generic instantiation internals.

## Phase 4: Link Bodies

### SymResLinkBody

This is the most complex visitor. It resolves ALL remaining symbols in function bodies, statements, and expressions.

#### What it resolves

| Category | Examples |
|----------|----------|
| Variable references | `x`, `self`, `param` |
| Function calls | `foo()`, `obj.method()` |
| Type references in expressions | `sizeof(int)`, `int as float` |
| Access chains | `a.b.c()`, `obj.field` |
| Move semantics | Tracking moved identifiers and chains |
| Operator overloads | `a + b` → `add(a, b)` |
| Implicit conversions | Pointer decay, reference binding |
| Pattern matching | `var Some(value) = opt else default` |
| Lambdas | Closure type, captures |
| Comptime blocks | Condition evaluation, if/else branch selection |

#### Move Semantics Tracking

The `SymResLinkBody` maintains two vectors:

```cpp
std::vector<VariableIdentifier*> moved_identifiers;  // moved single variables
std::vector<AccessChain*> moved_chains;               // moved access chains (e.g., obj.field)
```

When a variable is moved (assigned to another variable that has a destructor), it's added to one of these vectors. Subsequent access to the moved variable produces a compile error.

**Key functions:**

| Function | Purpose |
|----------|---------|
| `mark_moved_no_check(id/chain)` | Marks a value as moved |
| `un_move_id(id)` | Unmarks a moved identifier (for reassignment) |
| `find_moved_id(id)` | Checks if an identifier has been moved |
| `check_chain(chain, assigning)` | Validates access chain is not accessing a moved value |
| `check_id(id, diagnoser)` | Validates identifier is not accessing a moved value |
| `find_partially_matching_moved_chain(chain, ...)` | Find moved chains matching a prefix/path |
| `is_value_movable(value, type)` | Checks if a value can be moved (has destructor) |

The move semantics API uses functional equality: two identifiers match if their `linked` node points to the same declaration. Access chains with a matching first element also match.

### Operator Overload Resolution

Operator overloading is resolved in `SymResLinkBody` via `ImplementationsIndex`:

```cpp
// For a + b:
// 1. Determine the type of 'a'
// 2. Look up MembersContainer for that type
// 3. Call implsIndex.get_expr_op_impl(coreNodes, container, Operation::Add)
// 4. Replace the Expression AST node with a FunctionCall to the resolved 'add' method
```

The `ImplementationsIndex` uses a key of `(Interface*, ASTAny*)` where:
- `Interface*` is the interface declaration (e.g., `core::ops::Add`)
- `ASTAny*` is the type the impl is for (e.g., a `StructDefinition*` or `BaseType*`)

Thread safety: uses `std::shared_mutex` for concurrent reads (shared_lock) and exclusive writes (unique_lock).

## SymbolTable Internals

### SymbolTable

```cpp
class SymbolTable {
    std::vector<ScopeEntry> scopes;           // Nested scopes
    std::unordered_map<chem::string_view, BucketSymbol*> symbols;  // Hash map
};
```

### Scope Nesting

```
Global (permanent)
├── Module (module scope — multiple modules exist concurrently)
│   ├── File (file scope — symbols die after linking)
│   │   ├── Namespace (namespace scope)
│   │   ├── Struct (struct member scope)
│   │   ├── Function (function parameter scope)
│   │   └── Block (if/while/for block scope)
```

Key operations:
- `scope_start(kind)` — pushes a new scope marker
- `scope_end()` — pops the last scope
- `scope_start_index(kind)` — returns an index for later `drop_all_scopes_from(index)`
- `resolve(name)` — walks scopes from innermost to outermost, returns the first match
- `declare(name, node)` — declares in the current scope

### File and Module Scope Management

Private symbols in a file are stored in a `SymbolRange` (start/end index). When the file's linking is complete, the file scope is dropped, removing private symbols.

Internal symbols (visible to the module but not exported) are dropped when `module_scope_end()` is called.

## Import Resolution

### ImportPathHandler

Located in `preprocess/ImportPathHandler.h/.cpp`, this component:

1. Resolves local imports (`import "../lib_mod"`)
2. Resolves remote imports (`import "github.com/owner/repo"`)
3. Handles version pinning and conflict resolution
4. Handles orphan branches and subdirectories in monorepos
5. Handles conditional imports (`if windows`)

After resolving, it provides the path to the module's `chemical.mod` or `build.lab`, which the Lab build system uses to load the module.

## CoreNodes

Located in `compiler/symres/CoreNodes.h`, this provides all the core interfaces needed for operator overloading and built-in functionality:

- `core::ops::Add`, `Sub`, `Mul`, `Div`, `Mod` — arithmetic
- `core::ops::BitAnd`, `BitOr`, `BitXor`, `Shl`, `Shr` — bitwise
- `core::ops::Neg`, `Not`, `BitNot` — unary
- `core::ops::Index` — index operator
- `core::iterable::Iterable`, `ReversibleIterable` — for-in loops
- `core::iterable::ChunkedIterable`, `ReversibleChunkedIterable` — chunked iteration
- `core::ops::AssignAdd`, `AssignSub`, etc. — compound assignment
- `core::ops::Inc`, `Dec` — increment/decrement
- `core::serialization::StreamWrite` — serialization

These are linked during `link_core_nodes()` which is called before any user code is symbol-resolved.

## Common Issues and Debugging

### Diagnostic Collection

Each phase's visitor owns its own `ASTDiagnoser`. After the phase completes, diagnostics are merged:

```cpp
// Pattern used throughout:
result.diagnostics.insert(result.diagnostics.end(),
    std::make_move_iterator(visitor.diagnoser.diagnostics.begin()),
    std::make_move_iterator(visitor.diagnoser.diagnostics.end()));
```

### Parallel Debugging Tips

1. **Race conditions in generic instantiation**: Look for missing `generic_inst_reg_mutex` locks or missing shared_lock for `implsIndex`
2. **Missing symbols across files**: Check that the declaration phase successfully registered the symbol — the symbol table may not have been populated for that file
3. **Duplicate symbol errors**: Check for accidental double-declaration of the same name in the same scope
4. **Move semantics not catching**: Check that `is_value_movable()` returns true for the type — it requires `has_destructor()`
5. **Operator overload not found**: Check that the `impl` block was indexed in `ImplementationsIndex` — the interface and for-type must match exactly
6. **Per-file SymbolTable not populated**: Ensure the visitor's `table` is used for lookups, not the shared resolver's table

### Performance Considerations

- **SymbolTable lookups use `chem::string_view`** — no string copying
- **Per-file visitors reduce contention** — each file gets its own state
- **Generic instantiations are deduplicated** — the `InstantiationsContainer` ensures each (generic_decl, args) pair is only instantiated once
- **Diagnostics are batched** — stored in vectors, moved between phases, never copied
- **Use `shared_lock` for reads, `unique_lock` for writes** in `ImplementationsIndex`

## Code Map

| File | Lines (approx) | Purpose |
|------|---------------|---------|
| `compiler/symres/SymbolResolver.h` | ~250 | Core class declaration |
| `compiler/symres/SymbolResolver.cpp` | ~800 | `tld_declare_file`, `declare_and_link_file`, `import_file` |
| `compiler/symres/DeclareTopLevel.h` | ~40 | Top-level declaration visitor |
| `compiler/symres/SymResLinkBody.h` | ~300 | Body linking visitor + move API |
| `compiler/symres/SymResLinkBody.cpp` | ~4000 | Body linking implementation |
| `compiler/symres/LinkSignature.h` | ~200 | Signature linking visitor |
| `compiler/symres/LinkSignature.cpp` | ~2000 | Signature linking implementation |
| `compiler/symres/GenericInstantiationPass.h` | ~120 | Generic instantiation pass |
| `compiler/symres/GenericInstantiationPass.cpp` | ~120 | Generic instantiation pass impl |
| `compiler/symres/SymbolTable.h` | ~300 | Symbol table with scope nesting |
| `compiler/symres/ImplementationsIndex.h` | ~150 | Impl block index |
| `compiler/symres/CoreNodes.h` | ~100 | Core interfaces for operators |
| `compiler/symres/ChildResolver.h` | ~100 | Extension method resolver |
