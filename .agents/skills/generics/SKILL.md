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
    std::vector<GenericTypeParameter*> generic_params;  // <T, U, ...>
    GenericTypeParamsInfo paramsInfo;                    // Metadata about params
    ASTNode* generic_parent;                             // The AST node being specialized
    // ...
};
```

Each `GenericXxxDecl` holds:
1. **Generic type parameters** — the `<T, U, ...>` list
2. **The parent declaration** — the actual AST node (function body, struct members, etc.)
3. **Instantiation statuses** — tracks which instantiations are in progress/completed

## InstantiationsContainer

The `InstantiationsContainer` deduplicates generic instantiations:

```cpp
class InstantiationsContainer {
    // For each generic declaration, maps (concrete types) → concrete instantiation
    std::unordered_map<BaseGenericDecl*, std::vector<TypeInfo*>> instantiations;
    
    // Check if an instantiation already exists
    InstantiationInfo* find(BaseGenericDecl* gen, std::span<BaseType*> args);
    
    // Register a new instantiation
    InstantiationInfo* register_instantiation(BaseGenericDecl* gen, BaseType* result);
};
```

## Instantiation Process

### 1. Registration

When a `GenericType` (e.g., `Option<int>`) is encountered in a signature:

```cpp
// In GenericInstantiator::VisitGenericType():
void GenericInstantiator::VisitGenericType(GenericType* type) {
    // 1. Resolve the generic declaration
    auto* genDecl = type->resolveGenericDecl();
    
    // 2. Check if an instantiation with these args already exists
    auto* existing = container.find(genDecl, type->generic_args);
    if(existing) {
        type->setLinked(existing->result);  // Reuse existing
        return;
    }
    
    // 3. Register new instantiation
    auto* info = container.register(genDecl, type->generic_args);
    type->setLinked(info->result);
    
    // 4. Later: FinalizeSignature and FinalizeBody
}
```

### 2. Signature Finalization

```cpp
void GenericInstantiator::FinalizeSignature(GenericFuncDecl* genDecl, FunctionDeclaration* concrete, size_t itr) {
    // 1. Activate type parameter mapping for this instantiation
    activateIteration(genDecl, itr);
    
    // 2. Visit function signature (parameter types, return type)
    visit(concrete->returnType);
    for(auto& param : concrete->params) {
        visit(param);
    }
    
    // 3. Replace generic types with concrete types in the signature
    // (This modifies the concrete function's type in-place)
    
    // 4. Notify waiters that signature is finalized
    notifySignatureFinalized(genDecl, itr);
}
```

### 3. Body Finalization

```cpp
void GenericInstantiator::FinalizeBody(GenericFuncDecl* genDecl, FunctionDeclaration* concrete, size_t itr) {
    // 1. Re-activate the type parameter mapping
    activateIteration(genDecl, itr);
    
    // 2. Visit function body
    for(auto& stmt : concrete->body.nodes) {
        visit(stmt);
    }
    
    // 3. All generic type references in the body are now replaced
    // with concrete type references
}
```

### Thread Safety

Generic instantiation uses a **registration mutex** and **per-instantiation status**:

```cpp
struct InstantiationStatus {
    std::atomic<bool> signature_finalized{false};
    std::mutex status_mutex;
    std::condition_variable status_cv;
};
```

```cpp
// Wait until signature is finalized (called from FinalizeBody):
void GenericInstantiator::waitSignatureFinalized(BaseGenericDecl* decl, size_t index) {
    auto& status = decl->instantiation_statuses[index];
    std::unique_lock lock(status.status_mutex);
    status.status_cv.wait(lock, [&] { return status.signature_finalized.load(); });
}

// Notify that signature is finalized (called from FinalizeSignature):
void GenericInstantiator::notifySignatureFinalized(BaseGenericDecl* decl, size_t index) {
    auto& status = decl->instantiation_statuses[index];
    {
        std::lock_guard lock(status.status_mutex);
        status.signature_finalized.store(true);
    }
    status.status_cv.notify_all();
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
    // 1. Get the concrete type arguments for this iteration
    auto* typeInfo = container.get_type_info(genDecl, itr);
    
    // 2. Map each generic parameter to its concrete type
    current_gen = genDecl;
    active_type_map.clear();
    for(size_t i = 0; i < genDecl->generic_params.size(); i++) {
        active_type_map[genDecl->generic_params[i]] = typeInfo->args[i];
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

This pass runs between link signatures and link bodies:

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
enum class InstantiationRequirement {
    Registration,      // Only register the instantiation
    Signature,         // Also finalize the signature
    BodyComplete       // Also finalize the body
};
```

These control how deeply the instantiation is processed:

- **Registration**: Just record that this combination exists — no codegen yet
- **Signature**: Resolve all type references — needed for type checking
- **BodyComplete**: Full monomorphization — needed for codegen

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
