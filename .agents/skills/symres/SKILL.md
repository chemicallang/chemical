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
4. Automatic Function Gen   — generate_automatic_functions_for_module() creates implicit ctors/dtors
5. Generic Instantiation    — GenericInstantiationPass registers concrete instantiations
6. After Link Signature     — sym_res_after_signature() builds container indexes, links exports, computes generic param bits
7. Link Bodies              — sym_res_link_body_generic_decls_pass() then sym_res_link_body_pass() resolve all bodies
8. Type Verify              — type_verify() validates types (separate pass, in compiler/typeverify/)
```

### Key Files

| File | Purpose |
|------|---------|
| `compiler/symres/SymbolResolver.h` | Core `SymbolResolver` class — owns `SymbolTable`, `ChildResolver`, manages the per-phase API |
| `compiler/symres/SymbolResolver.cpp` | Implementation — `tld_declare_file()`, `link_signature_file()`, `generic_instantiation_file()`, `after_link_signature_file()`, `declare_and_link_file()`, `import_file()` |
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
    // 1. TopLevelDeclSymDeclare — declare all file symbols
    // 2. sym_res_signature() — link type signatures
    // 3. sym_res_generic_instantiation() — register/finalize generic instantiations
    // 4. sym_res_after_signature() — build container indexes, link exports, compute generic param bits
    // 5. sym_res_link_body_pass() — link all function bodies
    // 6. Merge diagnostics back
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
3. **Generic instantiation phase (parallel)**: Each file gets its own `GenericInstantiationPass` with a per-file `GenericInstantiatorAPI`
4. **After-link-signature phase (serial)**: `sym_res_after_signature()` builds container indexes, then links `export` statements and computes generic-param interface bits
5. **Body phase (parallel, two passes)**: `sym_res_link_body_generic_decls_pass()` resolves generic master bodies first, then `sym_res_link_body_pass()` resolves non-generic bodies. Each file gets its own `SymResLinkBody` with per-file `SymbolTable` and `GenericInstantiatorAPI`

The shared state that IS mutated:
- `generic_inst_reg_mutex` — protects generic instantiation registration maps
- `implsIndex.index_mutex` — a `std::shared_mutex` (shared_lock for reads, unique_lock for writes)
- `diagnostics` — merged back after each phase completes

## Phase 1: Top-Level Declaration

### TopLevelDeclSymDeclare

This visitor walks top-level declarations and registers them in the symbol table:

| Node Type | Declaration Behavior |
|-----------|---------------------|
| `FunctionDeclaration` | Declares by name with its access specifier; a duplicate name in the same scope reports a duplicate-symbol error |
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

#### Async return-type wrapping

`LinkSignature::visit_func_decl` wraps an `async func f(...) : T`'s parked return
type into `FutureHandle<T>` when **all** of these hold:

- the active backend lowers async (`lowers_async()` ∈ {`"C"`, `"LLVM"`});
- the `core::async` protocol is reachable (`coreNodes.async.future_handle != nullptr`);
- the `async` library (which supplies `chemical_async_frame_alloc`/`_free`) is
  imported. If only `core` is in scope, the function is **not** wrapped and the
  diagnoser emits `async functions require the async library; add import async`.

A `void` async result wraps `core::async::Unit` instead of `void`
(`FutureHandle<void>` is never formed). `FunctionDeclaration::inner_return_type()`
unwraps the handle back to `T`, and `attrs.is_async` marks the declaration for the
backends and the type verifier. `AwaitExpression` is linked in the body pass via
`SymResLinkBody::VisitAwaitExpression` (it unwraps the handle to the inner type).

> **Pending symres/generics async fixes (worked around, not fixed):** B20
> (composite-generic field access inside a generic body; partially fixed) and
> async-closure lowering. See `lang/docs/async-remaining-work.md` before
> removing the runtime's type-erasure workarounds.

## Phase 3: Generic Instantiation Pass

### GenericInstantiationPass

This pass runs BETWEEN link signatures and link bodies (after automatic function generation). It:

1. Finalizes inline instantiations from the link signature phase (calls `GenericTypeDecl::finalize_signature()`)
2. Visits scopes, namespaces, comptime-if scopes, struct values, and function signatures, requesting registration of each `GenericType` it encounters (`InstantiationRequirement::Registration`)
3. Explicitly does NOT visit generic declarations themselves, and does NOT visit function bodies — only signatures

This separation allows the generic instantiation pass to use a parallel model where each instantiation can be finalized independently.

See [generics skill](./.agents/skills/generics/SKILL.md) for detailed generic instantiation internals.

## Phase 4: After Link Signature

### sym_res_after_signature

This pass runs after generic instantiation and before link bodies (`sym_res_after_signature()` in `compiler/symres/LinkSignature.cpp`). It:

1. Calls `BuildIndexes()` — builds container→children indexes so bodies can look up members
2. Runs `AfterBuildIndexesPass()` — links `export` statements (which can't be linked during signature) and computes interface bits of generic type parameters

## Phase 5: Link Bodies

### SymResLinkBody

This is the most complex visitor. It resolves ALL remaining symbols in function bodies, statements, and expressions.

In the parallel module pipeline it runs in two passes: `sym_res_link_body_generic_decls_pass()` resolves the master bodies of generic declarations first, then `sym_res_link_body_pass()` resolves all non-generic bodies.

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
| `check_chain(chain, assigning, diagnoser)` | Validates access chain is not accessing a moved value |
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

The `ImplementationsIndex` uses an `ImplementationIndexKey` of `(ASTNode*, ASTAny*)` where:
- `ASTNode*` is the interface declaration — an `InterfaceDefinition*` or a `GenericInterfaceDecl*` (`add_interface` normalizes a generic interface to its parent declaration)
- `ASTAny*` is the type the impl is for (e.g., a `BaseType*`, `StructDefinition*`, `VariantDefinition*`, or `UnionDef*`)

Thread safety: uses `std::shared_mutex` for concurrent reads (shared_lock) and exclusive writes (unique_lock).

## SymbolTable Internals

### SymbolTable

```cpp
class SymbolTable {
    std::vector<SymbolEntry> symbols;    // Contiguous metadata for declared symbols
    std::vector<Bucket> buckets;         // Open-addressed hash table (BucketSymbol chains)
    size_t bucketMask;                   // buckets.size() - 1 (bucket count is a power of two)
    std::vector<SymbolScope> scopeStack; // Scope markers (SymResScopeKind + start index)
};
```

`SymResScopeKind` only distinguishes `Global`, `Module`, `File`, and `Default`; namespaces, struct members, function parameters, and blocks all use `Default` scopes.

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

## CoreNodes — How Core Interfaces are Bound

Located in `compiler/symres/CoreNodes.h`, this provides all the core interfaces needed for operator overloading and built-in functionality. These are linked at runtime during `SymbolResolver::link_core_nodes()` (defined in `compiler/symres/SymbolResolver.cpp`).

### The Binding Mechanism

`link_core_nodes()` is called after a module completes symbol resolution, but only for the top-level `core` module (`module->scope_name.empty() && module->name == "core"`, checked in `ASTProcessor::sym_res_module()`). It looks up interfaces by name in the symbol table and stores function pointers in the `CoreNodes` struct:

```cpp
void SymbolResolver::link_core_nodes() {
    // Step 1: Find the Copy marker interface
    const auto copyNode = find("Copy");
    if(copyNode && copyNode->kind() == ASTNodeKind::InterfaceDecl) {
        coreNodes.copy_interface = copyNode->as_interface_def_unsafe();
        coreNodes.copy_interface->interface_bits.set(InterfaceBits::COPY_BIT);
    }

    // Step 2: Find the core namespace
    const auto coreNode = find("core");
    if(!coreNode || coreNode->kind() != ASTNodeKind::NamespaceDecl) return;

    // Step 3: Find core::ops, core::iterable, core::stream
    const auto opsNode = coreNode->child("ops");
    const auto iterableNode = coreNode->child("iterable");
    const auto streamNode = coreNode->child("stream");

    // Step 4: Bind each interface method using func_of_interface()
    coreNodes.ops.add = func_of_interface(opsNode, "Add", "add");
    coreNodes.ops.sub = func_of_interface(opsNode, "Sub", "sub");
    // ... etc for all operators
}
```

The `func_of_interface()` helper resolves an interface method:

```cpp
static FunctionDeclaration* func_of_interface(
    ASTNode* container,        // The namespace containing interfaces
    const chem::string_view& interface,  // Interface name, e.g. "Add"
    const chem::string_view& method      // Method name, e.g. "add"
) {
    const auto found = container->child(interface);
    if(!found) return nullptr;
    const auto child = found->child(method);
    if(!child || child->kind() != ASTNodeKind::FunctionDecl) return nullptr;
    return child->as_function_unsafe();
}
```

### The `CoreNodes` Struct Hierarchy

```
CoreNodes
├── copy_interface — The `Copy` interface (marker for trivially copyable types)
├── CoreNodesOps — Operator overload functions
│   ├── Arithmetic: add, sub, mul, div, rem
│   ├── Bitwise: bit_and, bit_or, bit_xor, shl, shr
│   ├── Unary: neg, not, bitnot
│   ├── Compound assignment: add_assign, sub_assign, mul_assign, div_assign, rem_assign
│   │                bit_and_assign, bit_or_assign, bit_xor_assign, shl_assign, shr_assign
│   ├── Comparison: eq, ne (from PartialEq)
│   │               gt, lt, gte, lte (from Ord)
│   ├── Inc/Dec: inc_pre, inc_post, dec_pre, dec_post (from Increment/Decrement)
│   └── Index: index, index_mut (from Index/IndexMut)
├── CoreNodesIterable — Iteration interface functions
│   ├── LinearIterable: linear_data, linear_size
│   ├── ChunkedIterable: begin_chunks, valid_chunk, current_chunk, next_chunk
│   │                    rbegin_chunks, previous_chunk, total_size
│   ├── Iterable: begin, valid, current, next
│   └── ReversibleIterable: rbegin, previous, count
└── CoreNodesStream — Serialization interface functions
    └── Stream: writeSigned, writeUnsigned, writeStr, writeStrNoLen,
                writeFloat, writeDouble, writeChar, writeUChar
```

### How `Copy` Interface Works

The `Copy` interface is a special marker interface. When a struct implements `core::ops::Copy`, the compiler marks it with `InterfaceBits::COPY_BIT`. This bit is checked during move semantics:

```cpp
// If a type implements Copy, it can be duplicated without move semantics
// If it doesn't implement Copy and has a destructor, moves are enforced
```

### How Operator Overloads Use CoreNodes

During SymResLinkBody (Phase 5), when an expression like `a + b` is encountered:

```cpp
// In SymResLinkBody expression handling:
// 1. Get the type of 'a'
// 2. Get the MembersContainer for that type
// 3. Look up the impl for core::ops::Add for that type:
FunctionDeclaration* add_impl = implsIndex.get_expr_op_impl(coreNodes, container, Operation::Add);
// 4. If found, rewrite the Expression to a FunctionCall to add_impl
// 5. If not found, report error: "operator + not supported for type X"
```

The `ImplementationsIndex` provides methods like:
- `get_expr_op_impl(coreNodes, container, op)` — for binary operators
- `get_ass_op_impl(coreNodes, container, op)` — for compound assignment operators
- `get_inc_dec_op_impl(coreNodes, container, increment, post)` — for ++/--
- `get_index_op_impl(coreNodes, container)` — for [] operator
- `get_neg_op_impl(coreNodes, container)` — for unary -
- `get_iterable_begin_impl(coreNodes, container)` — for for-in iteration

### Complete Interface List

| Interface | Namespace | Method | Operator/Syntax |
|-----------|-----------|--------|-----------------|
| `Add` | `core::ops` | `add` | `a + b` |
| `Sub` | `core::ops` | `sub` | `a - b` |
| `Mul` | `core::ops` | `mul` | `a * b` |
| `Div` | `core::ops` | `div` | `a / b` |
| `Rem` | `core::ops` | `rem` | `a % b` |
| `Neg` | `core::ops` | `neg` | `-a` |
| `Not` | `core::ops` | `not` | `!a` |
| `BitNot` | `core::ops` | `bitnot` | `~a` |
| `BitAnd` | `core::ops` | `bitand` | `a & b` |
| `BitOr` | `core::ops` | `bitor` | `a | b` |
| `BitXor` | `core::ops` | `bitxor` | `a ^ b` |
| `Shl` | `core::ops` | `shl` | `a << b` |
| `Shr` | `core::ops` | `shr` | `a >> b` |
| `PartialEq` | `core::ops` | `eq`, `ne` | `a == b`, `a != b` |
| `Ord` | `core::ops` | `gt`, `lt`, `gte`, `lte` | `a > b`, `a < b`, etc. |
| `Increment` | `core::ops` | `inc_pre`, `inc_post` | `++a`, `a++` |
| `Decrement` | `core::ops` | `dec_pre`, `dec_post` | `--a`, `a--` |
| `Index` | `core::ops` | `index` | `a[b]` (read) |
| `IndexMut` | `core::ops` | `index` | `a[b] = val` (write) |
| `AddAssign` | `core::ops` | `add_assign` | `a += b` |
| `SubAssign` | `core::ops` | `sub_assign` | `a -= b` |
| `MulAssign` | `core::ops` | `mul_assign` | `a *= b` |
| `DivAssign` | `core::ops` | `div_assign` | `a /= b` |
| `RemAssign` | `core::ops` | `rem_assign` | `a %= b` |
| `BitAndAssign` | `core::ops` | `bitand_assign` | `a &= b` |
| `BitOrAssign` | `core::ops` | `bitor_assign` | `a |= b` |
| `BitXorAssign` | `core::ops` | `bitxor_assign` | `a ^= b` |
| `ShlAssign` | `core::ops` | `shl_assign` | `a <<= b` |
| `ShrAssign` | `core::ops` | `shr_assign` | `a >>= b` |
| `Iterable` | `core::iterable` | `begin`, `valid`, `current`, `next` | `for x in container` |
| `ReversibleIterable` | `core::iterable` | `rbegin`, `previous`, `count` | `for x in container reversed` |
| `Linear` | `core::iterable` | `data`, `size` | Direct data access |
| `Chunked` | `core::iterable` | `begin_chunks`, `valid_chunk`, etc. | Chunked iteration |
| `Copy` | top-level | (marker interface) | Move semantics exception |
| `Stream` | `core::stream` | `writeSigned`, `writeUnsigned`, etc. | Serialization |

### Why `link_core_nodes()` is Called Per-Core-Module

CoreNodes are bound only when the compiler processes the top-level `core` module (`module->scope_name.empty() && module->name == "core"`, checked in `ASTProcessor::sym_res_module()`). This is because the core interfaces are defined in Chemical source code in `lang/libs/core/`. The binding happens AFTER symbol resolution of the core module is complete, so all interfaces are visible in the symbol table.

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
| `compiler/symres/SymbolResolver.h` | ~510 | Core class declaration |
| `compiler/symres/SymbolResolver.cpp` | ~430 | `tld_declare_file`, `link_signature_file`, `generic_instantiation_file`, `after_link_signature_file`, `declare_and_link_file`, `import_file` |
| `compiler/symres/DeclareTopLevel.h` | ~60 | Top-level declaration visitor |
| `compiler/symres/SymResLinkBody.h` | ~640 | Body linking visitor + move API |
| `compiler/symres/SymResLinkBody.cpp` | ~3750 | Body linking implementation |
| `compiler/symres/LinkSignature.h` | ~270 | Signature linking visitor |
| `compiler/symres/LinkSignature.cpp` | ~1730 | Signature linking implementation (incl. `sym_res_after_signature`) |
| `compiler/symres/GenericInstantiationPass.h` | ~110 | Generic instantiation pass |
| `compiler/symres/GenericInstantiationPass.cpp` | ~70 | Generic instantiation pass impl |
| `compiler/symres/SymbolTable.h` | ~770 | Symbol table with scope nesting |
| `compiler/symres/ImplementationsIndex.h` | ~140 | Impl block index |
| `compiler/symres/CoreNodes.h` | ~200 | Core interfaces for operators |
| `compiler/symres/ChildResolver.h` | ~100 | Extension method resolver |
