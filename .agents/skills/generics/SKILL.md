---
name: Generics
description: Comprehensive guide to the Chemical generic instantiation system — how generic types, functions, and interfaces are monomorphized, finalized, and optimized for parallel execution.
---

# Generic Instantiation System

Chemical uses monomorphization (static instantiation) for generics. Each concrete combination of generic type parameters produces a separate copy of the generic declaration. This system runs during symbol resolution (between link signatures and link bodies).

## Architecture

### Pipeline

```
Generic Type Usage → Register Instantiation → Finalize Signature → Finalize Body → Concrete Type
```

### Key Files

| File | Purpose |
|------|---------|
| `compiler/generics/GenericInstantiator.h` | Core `GenericInstantiator` class — monomorphization visitor |
| `compiler/generics/GenericInstantiator.cpp` | Implementation — all `FinalizeSignature`/`FinalizeBody` methods |
| `compiler/generics/GenInstantiatorAPI.h` | API wrapper — provides thread-safe instantiation interface |
| `compiler/generics/InstantiationsContainer.h` | Container — deduplicates and tracks all instantiations |
| `compiler/symres/GenericInstantiationPass.h/.cpp` | Pass that triggers instantiation for generic types in signatures |

## Generic Declarations

Chemical supports generics on:

| Generic Declaration | Concrete Output | File Reference |
|---------------------|----------------|----------------|
| `GenericFuncDecl` | `FunctionDeclaration` | `ast/structures/GenericFuncDecl.h` |
| `GenericStructDecl` | `StructDefinition` | `ast/structures/GenericStructDecl.h` |
| `GenericUnionDecl` | `UnionDef` | `ast/structures/GenericUnionDecl.h` |
| `GenericInterfaceDecl` | `InterfaceDefinition` | `ast/structures/GenericInterfaceDecl.h` |
| `GenericVariantDecl` | `VariantDefinition` | `ast/structures/GenericVariantDecl.h` |
| `GenericImplDecl` | `ImplDefinition` | `ast/structures/GenericImplDecl.h` |

### The Generic Declaration Structure

```cpp
class BaseGenericDecl : public ASTNode {
    std::vector<GenericTypeParameter*> generic_params;            // <T, U, ...>
    std::vector<InstantiationStatusEntry> instantiation_statuses;  // Per-instantiation sync state
    // ...
};
```

Each `GenericXxxDecl` holds:
1. **Generic type parameters** — the `<T, U, ...>` list (`generic_params`)
2. **The master declaration** — `master_impl`, the AST node being specialized (function body, struct members, etc.)
3. **Instantiation statuses** — `instantiation_statuses`, tracking each instantiation's registration/signature progress

Concrete instantiations (e.g. a `StructDefinition` created from a `GenericStructDecl`) store a back-pointer `generic_parent` to the `GenericXxxDecl` that produced them.

## InstantiationsContainer

The `InstantiationsContainer` deduplicates generic instantiations:

```cpp
class InstantiationsContainer {
    // Generic declaration (or type) key → its registered instantiations
    std::unordered_map<void*, DeclInstantiations> instantiations;
    // File id → records of which instantiations that file created (for invalidation)
    std::unordered_map<unsigned int, std::vector<RegistryEntry>> fileIdRegistry;

    // Types already registered for a key
    std::span<InstantiationType> getInstantiationTypesFor(void* key);

    // Register a new instantiation, returns its index within the key's list
    size_t registerInstantiation(void* key, InstantiationType types,
                                 std::vector<void*>& instVec, unsigned int current_file_id);

    // Remove instantiations for a decl key / for a file
    void removeInstantiationsFor(void* key);
    void removeInstantiationsFor(unsigned int fileId);
};
```

Deduplication is performed by `register_generic_usage()` (`ast/utils/ASTUtils.cpp`): it calls `get_iteration_for()` to reuse an existing index when the args match, otherwise registers a new one via `registerInstantiation()`. The file registry lets the IDE drop instantiations created by an edited file.

## Instantiation Process

### 1. Registration

When a `GenericType` (e.g., `Option<int>`) is encountered in a signature:

```cpp
// In GenericInstantiator::VisitGenericType():
void GenericInstantiator::VisitGenericType(GenericType* type) {
    // 1. Instantiate the type arguments first (visit each type->types entry)

    // 2. Switch on the linked declaration's kind (GenericStructDecl, GenericUnionDecl,
    //    GenericInterfaceDecl, GenericVariantDecl, GenericTypeDecl) and call
    //    linked->instantiate_type(genApi, type->types, location, requirement)

    // 3. The returned concrete node replaces type->referenced->linked

    // 4. Self-referential case: when linked == current_gen and the args are still the
    //    generic params, relink to current_impl_ptr instead of recursing
}
```

### 2. Signature Finalization

```cpp
void GenericInstantiator::FinalizeSignature(GenericFuncDecl* genDecl, FunctionDeclaration* concrete, size_t itr) {
    // 1. Activate type parameter mapping for this instantiation
    activateIteration(genDecl, itr);
    
    // 2. Visit function signature (parameter types, then return type)
    for(auto& param : concrete->params) {
        visit(param);
    }
    visit(concrete->returnType);
    
    // 3. Replace generic types with concrete types in the signature
    // (This modifies the concrete function's type in-place)
    
    // 4. Waiters are notified by Generic*Decl::register_generic_args() via
    //    notifySignatureFinalized() — not by FinalizeSignature itself
}
```

### 3. Body Finalization

```cpp
void GenericInstantiator::FinalizeBody(GenericFuncDecl* genDecl, FunctionDeclaration* concrete, size_t itr) {
    // 1. Re-activate the type parameter mapping and set the current context
    current_gen = genDecl;
    current_impl_ptr = concrete;
    activateIteration(genDecl, itr);

    // 2. Visit function body (if present) inside a fresh symbol scope with params declared
    if(concrete->body.has_value()) {
        table.scope_start();
        for(const auto param : concrete->params) {
            table.declare(param->name_view(), param);
        }
        current_func_type = concrete;
        visit(concrete->body.value());
        table.scope_end();
    }

    // 3. All generic type references in the body are now replaced
    // with concrete type references
}
```

### Thread Safety

Generic instantiation uses a **registration mutex** and **per-instantiation status entries**. The status mutex and condition variable are shared, owned by the `InstantiationsContainer` (a single mutex + a single condition variable for all instantiations):

```cpp
enum class InstantiationStatus : uint8_t {
    Registered,
    SignatureFinalized
};

struct InstantiationStatusEntry {
    InstantiationStatus status;
    std::thread::id builder_thread;   // thread that registered this instantiation
};
```

```cpp
// Wait until signature is finalized (called before body finalization):
void GenericInstantiator::waitSignatureFinalized(BaseGenericDecl* decl, size_t index) {
    auto& status_mutex = container.getInstantiationStatusMutex();
    auto& cv = container.getInstantiationCv();
    std::unique_lock<std::mutex> lock(status_mutex);

    // do not wait on ourselves: recursive generics would deadlock against this thread
    if(decl->instantiation_statuses[index].builder_thread == std::this_thread::get_id()) return;

    cv.wait(lock, [decl, index]() {
        return decl->instantiation_statuses[index].status == InstantiationStatus::SignatureFinalized;
    });
}

// Notify that signature is finalized (called from register_generic_args):
void GenericInstantiator::notifySignatureFinalized(BaseGenericDecl* decl, size_t index) {
    auto& status_mutex = container.getInstantiationStatusMutex();
    auto& cv = container.getInstantiationCv();
    {
        std::lock_guard<std::mutex> lock(status_mutex);
        decl->instantiation_statuses[index].status = InstantiationStatus::SignatureFinalized;
    }
    cv.notify_all();
}
```

This allows:
- **Multiple threads** to finalize different instantiations of the same generic concurrently
- **Signature-first**: One thread finalizes the signature first, others wait
- **Body parallel**: After signature is done, multiple threads can finalize bodies in parallel

## The GenericInstantiator Visitor

`GenericInstantiator` extends `RecursiveVisitor` and is the workhorse of monomorphization.

### Key Methods

```cpp
class GenericInstantiator : public RecursiveVisitor<GenericInstantiator> {
    // Type parameter mapping — replaces generic params with concrete types
    std::unordered_map<GenericTypeParameter*, BaseType*> active_type_map;
    
    // Thread-local state
    BaseGenericDecl* current_gen = nullptr;   // What we're instantiating
    FunctionTypeBody* current_func_type = nullptr;
};
```

### What Gets Replaced

During instantiation, the visitor replaces:

| Generic AST Node | Replacement |
|------------------|-------------|
| `GenericType` (e.g., `T`) | Concrete type (e.g., `int`) |
| `VariableIdentifier` pointing to generic params | Relinked to concrete param |
| `FunctionCall` to generic functions | Relinked to concrete function |
| `StructValue` with generic member types | Concrete member types |
| `LinkedType` pointing to generic declarations | Linked to concrete instantiation |

### The `active_type_map`

This is the thread-local mapping from `GenericTypeParameter*` to `BaseType*`:

```cpp
void GenericInstantiator::activateIteration(BaseGenericDecl* genDecl, size_t itr) {
    // 1. Under registration_mutex, get this iteration's concrete type arguments
    std::lock_guard<std::recursive_mutex> lock(registration_mutex);
    auto instantiations = container.getInstantiationTypesFor(genDecl);
    auto types = instantiations[itr];

    // 2. Map each generic parameter to its concrete type
    current_gen = genDecl;
    active_type_map.clear();
    for(size_t i = 0; i < genDecl->generic_params.size(); i++) {
        active_type_map[genDecl->generic_params[i]] = types[i];
    }
}
```

## The `SymResSignatureResult`

During link signature, `SymResSignatureResult` collects all the inline generic instantiations encountered:

```cpp
struct SymResSignatureResult {
    std::vector<std::pair<TypealiasStatement*, std::vector<TypeLoc>>> inline_instantiations;
    // ... diagnostics, errors
};
```

These are processed after the link signature pass by `GenericInstantiationPass`.

## GenericInstantiationPass

This pass runs between link signatures and link bodies (a subsequent `sym_res_after_signature()` pass runs after it, before body linking):

```cpp
GenInstSignatureResult sym_res_generic_instantiation(SymbolResolver& resolver, Scope* scope, SymResSignatureResult& result, const SymbolRange& range) {
    // 1. Finalize inline instantiations from link signature
    for(auto& inst : result.inline_instantiations) {
        GenericTypeDecl::finalize_signature(allocator, inst.first);
    }
    
    // 2. Finalize signatures of all registered instantiations
    for(auto& inst : result.inline_instantiations) {
        visitor.generic_instantiator.FinalizeSignature(inst.first->generic_parent, inst.first, inst.second);
    }
    
    // 3. Visit the entire scope to trigger nested generic instantiations
    visitor.visit(scope);
    
    return GenInstSignatureResult { ... };
}
```

**Important**: This pass only handles **registration** of instantiations (`InstantiationRequirement::Registration`). It does NOT require signature or body finalization of nested instantiations it discovers.

## InstantiationRequirements

```cpp
enum class InstantiationRequirement : uint8_t {
    Registration,          // Only register the instantiation
    SignatureFinalization  // Register + finalize the signature (and wait for deps)
};
```

These control how deeply the instantiation is processed:

- **Registration**: Just record that this combination exists — no signature finalization (used by `GenericInstantiationPass`)
- **SignatureFinalization**: Register plus resolve all type references in the signature; body finalization follows automatically once the generic declaration's body has been linked (`body_linked`)

## CoreNodes and Impl Index

Generic instantiation depends heavily on:

1. **CoreNodes** — Provides the core interfaces (`core::ops::Add`, etc.) that generic code uses for operator overloading
2. **ImplementationsIndex** — Finds `impl` blocks that implement interfaces for concrete types

```cpp
// During instantiation, when resolving a + b:
// 1. Determine concrete types of a and b
// 2. Find impl for core::ops::Add for those types
// 3. Rewrite a + b to add(a, b) with concrete types
```

## Common Issues

### 1. Self-Referential Generic Declarations

```chemical
struct Node<T> {
    var value : T
    var next : *Node<T>  // Self-referential
}
```

This works because:
- The pointer `*Node<T>` is a reference to the same instantiation
- The `current_gen` check prevents infinite recursion by recognizing self-references

### 2. Missing Impl During Instantiation

If an `impl` block for a concrete type hasn't been indexed yet:

```
Error: cannot find implementation of interface 'Add' for type 'MyStruct<int>'
```

**Fix**: Ensure the `impl` block is in the symbol table before the generic function using it is instantiated.

### 3. Generic Recursion Limit

Deeply nested generic instantiations can be expensive:

```chemical
var x = foo<bar<baz<int>>>()  // 3 levels of instantiation
```

The compiler handles this but may be slow for extremely deep nesting.

### 4. Thread Safety Issues

**Symptom**: Intermittent crashes during parallel generic instantiation
**Cause**: Missing `registration_mutex` lock or not using `shared_lock` for `implsIndex`
**Fix**: Ensure all registration mutations hold `std::lock_guard(registration_mutex)` and reads use `std::shared_lock`

## Performance Considerations

1. **Deduplication**: `InstantiationsContainer` ensures each unique combination of generic args is only instantiated once
2. **Parallel finalization**: Different instantiations can be finalized concurrently
3. **Signature-first**: Body finalization blocks on signature, allowing parallel body finalization
4. **Thread-local state**: The `active_type_map` avoids data races on shared `GenericTypeParameter` state
5. **Arena allocation**: All instantiated AST nodes use `ASTAllocator` arena allocation — no per-node `delete`
