---
name: LLVM Backend
description: Comprehensive guide to the LLVM codegen backend — how Chemical AST is lowered to LLVM IR, key patterns, gotchas, debugging, and parallelization strategies.
---

# LLVM Backend

The LLVM backend is the primary optimizing codegen path for the Chemical compiler. It translates the fully resolved and type-checked AST into LLVM IR, which LLVM then optimizes and lowers to machine code.

## Architecture Overview

### Pipeline

```
Type-checked AST → LLVM.cpp (expression/value lowering) → LLVMGen.cpp (IR builder helpers) → LLVM IR → LLVM optimization passes → Object code
```

### Key Files

| File | Purpose |
|------|---------|
| `compiler/backend/LLVM.cpp` | Main codegen — converts AST values, types, and expressions to LLVM IR |
| `compiler/backend/LLVMGen.cpp` | IR builder utilities — allocation, GEP, stores, function creation |
| `compiler/backend/LLVMGen.h` | LLVMGen class declaration |
| `compiler/backend/LLVMBackendContext.h` | Backend context — holds LLVMContext, Module, IRBuilder, target machine |
| `compiler/backend/LLVMBackendContext.cpp` | Context implementation |
| `compiler/backend/DebugInfoBuilder.h/.cpp` | Debug info (DWARF) generation |
| `compiler/backend/include/LLVMArrayDestructor.h` | Array destructor helpers |
| `compiler/backend/CLANG.cpp` | Clang integration for C driver mode |
| `compiler/backend/LLVMTypes.cpp` | Type mapping from Chemical types to LLVM types |
| `compiler/backend/LLVMModuleEmitter.cpp` | Module emission (writing IR/object files) |
| `compiler/backend/LLVMTargetHelper.cpp` | Target architecture helpers |

## LLVMBackendContext

The `LLVMBackendContext` class manages all LLVM state for compilation:

```cpp
class LLVMBackendContext {
    llvm::LLVMContext llvmContext;        // LLVM context
    std::unique_ptr<llvm::Module> module;  // LLVM module
    llvm::IRBuilder<> builder;             // IR builder
    llvm::TargetMachine* targetMachine;    // Target machine description
    // ... plus debug info, function maps, type maps, etc.
};
```

### Key Responsibilities

1. **Creating LLVM functions** — mapping Chemical functions to LLVM functions
2. **Creating LLVM global variables** — `llvm::GlobalVariable` with proper linkage
3. **Managing IR builder** — insertion point, current function
4. **Debug info** — managing `DIBuilder` for source-level debugging
5. **Type cache** — memoizing Chemical → LLVM type conversions

## LLVM Codegen: Key Patterns

### Type Lowering

Chemical types are mapped to LLVM types via a visitor pattern:

| Chemical Type | LLVM Type | Notes |
|---------------|-----------|-------|
| `int` (i32) | `i32` | Direct mapping |
| `i8`, `i16`, `i64` | `i8`, `i16`, `i64` | Direct mapping |
| `float` | `float` | Direct mapping |
| `double` | `double` | Direct mapping |
| `bool` | `i1` | Zero-extended to `i8` for storage |
| `*T` | `T*` | LLVM pointer type |
| `&T` | `T*` | Lowered to pointer |
| `[N]T` | `[N x T]` | LLVM array type |
| `struct S` | `{ T1, T2, ... }` | LLVM struct type |
| `variant V` | `{ i8, { ... } }` | Tagged union (discriminator + payload) |
| `func (P) → R` | `R(*)(P)` | Function pointer |
| `void` | `void` | Only for function returns |

### Struct Lowering

Structs are the most complex lowering target:

1. **Simple structs** → LLVM named struct type with packed/non-packed layout
2. **Struct with destructor** → generates an `@llvm.memcpy` pattern for assignment (see gotchas)
3. **Struct with constructor** → generates constructor function called at initialization
4. **Struct return values** → lowered to sret (struct return) pointer parameter

### Function Lowering

Chemical functions are lowered following C ABI conventions:

1. **Name mangling**: Scoped → `scope_name` prefix, generics → `__cgs__N`/`__cfg__N` suffix
2. **Parameters**: Direct mapping, with sret for struct returns
3. **Main function**: Not mangled for `application` packages
4. **External functions**: `@extern` → no mangling, external linkage

### Control Flow Lowering

| Construct | LLVM Pattern |
|-----------|--------------|
| `if/else` | `icmp` + `br cond` → blocks with `phi` for if-expression results |
| `while` | `br` → header block → `icmp` + `br` → body/exit |
| `for` | init block → `br` → header → body → increment → header |
| `switch` | `switch` instruction with cases |
| `break` | `br` to after-loop block |
| `continue` | `br` to loop header |

## LLVM Gotchas

### 1. `dso_local` and External Declarations

**Rule**: When a global variable is declared in another module (`submod_extern_globe_var`), do **NOT** set `dso_local` on the LLVM global value.

```cpp
// WRONG — linker rejects dso_local + external:
auto* gv = new llvm::GlobalVariable(*module, type, false,
    llvm::GlobalValue::ExternalLinkage, nullptr, name);
gv->setDSOLocal(true);  // BUG: linker error for cross-module references

// CORRECT:
auto* gv = new llvm::GlobalVariable(*module, type, false,
    llvm::GlobalValue::ExternalLinkage, nullptr, name);
// No setDSOLocal() call
```

Relevant files: `ast/statements/VarInit.cpp`, `compiler/backend/LLVM.cpp`

### 2. Struct Assignment: Temp + Destruct + Memcpy Pattern

The LLVM backend assigns structs via a three-step pattern:

```llvm
; Step 1: Bitwise copy source into a stack temp
%temp = alloca %struct.Type
call void @llvm.memcpy.p0.p0.i64(%temp, %src, size, align)

; Step 2: Call destructor on destination
call void @Type_destruct(%dest)

; Step 3: memcpy temp onto destination
call void @llvm.memcpy.p0.p0.i64(%dest, %temp, size, align)
```

**This breaks self-referencing pointers**. If a struct has a pointer field pointing to one of its own members (e.g., `function`'s `fn_data_ptr`), the bitwise copy produces a dangling pointer — the temp shares the same pointer, but after `memcpy` over the destination, the temp is destroyed. The destination's pointer now points to freed memory.

**Possible fixes** (none implemented yet):
- `@reflat` annotation to skip destruct+memcpy
- `@move` hook for custom move semantics
- Forbid self-referencing pointers inside value types

### 3. Uninitialized Variables and PHI Nodes

LLVM requires `UndefValue` for uninitialized phis:

```cpp
// CORRECT pattern:
auto* phi = builder.CreatePHI(type, numIncoming);
phi->addIncoming(val1, block1);
phi->addIncoming(UndefValue::get(type), block2);  // uninitialized path
```

Check `IRBuilder::CreatePHI` usage in `LLVM.cpp` when adding new PHI-based constructs.

### 4. Alloca Placement

All `alloca` instructions should be at the start of the entry block, not scattered throughout the function. Use `LLVMGen::CreateEntryBlockAlloca()`:

```cpp
// In LLVMGen.cpp:
AllocaInst* CreateEntryBlockAlloca(Function* function, Type* type, const Twine& name = "") {
    IRBuilder<> tmpBuilder(&function->getEntryBlock(), function->getEntryBlock().begin());
    return tmpBuilder.CreateAlloca(type, nullptr, name);
}
```

### 5. GEP (GetElementPtr) Indices

When accessing struct fields, the GEP indices must be correct:

```cpp
// For struct S { int a; float b; }
// Accessing 'b' via S* ptr:
// GEP indices: [0, 1]  (0 = deref pointer, 1 = field index)
auto* gep = builder.CreateStructGEP(ptr, 1);  // field index 1 = b
```

### 6. Variant (Tagged Union) Lowering

Variants are lowered as:

```llvm
%variant = type { i8, %union_data }  ; i8 = discriminator, union_data = payload
```

- `i8 discriminator` at offset 0 — which case is active
- `%union_data` at offset aligned to largest member — the payload is a `{T1, T2, ...}` union

Pattern matching generates:
1. Load discriminator
2. `icmp eq` with case value
3. `br` to matching case block

## Debug Info Generation

The `DebugInfoBuilder` generates DWARF debug information:

- **Source locations**: Every LLVM instruction has debug metadata pointing to the source position
- **Function debug info**: `DISubprogram` for each Chemical function
- **Variable debug info**: `DILocalVariable` for local variables
- **Type debug info**: `DIDerivedType`, `DICompositeType` for structs, arrays, pointers

```cpp
// Pattern:
DILocalVariable* var = debugInfo->createLocalVariable(name, fn, line, type, true, 0);
debugInfo->insertValue(var, alloca, location);
```

## Parallelization Strategies

Currently, the LLVM backend runs **per-function** and **per-module** in a mostly serial fashion within each compilation job. Future parallelization opportunities:

1. **Per-function codegen**: Each function's LLVM IR generation could run in parallel, with shared context
2. **Per-module codegen**: If a job compiles multiple modules, each module's codegen is independent
3. **LLVM optimization passes**: LLVM's own `ModulePassManager` can parallelize within a module

**Current limitation**: `llvm::IRBuilder<>` is not thread-safe per-context. Options:
- Use multiple `LLVMContext` instances (one per thread)
- Use `llvm::orc::ThreadSafeModule` for thread-safe module ownership

## Debugging LLVM IR

### Dumping IR

```bash
# Pass --out-ll-all to dump LLVM IR for all modules:
cmake-build-debug/Compiler "lang/compiled/temp.ch" --out-ll-all --build-dir "lang/compiled" -o "lang/compiled/temp.exe" --mode debug_complete --debug-ir
```

IR is written to `lang/compiled/modules/main/llvm_ir.ll`.

### Common LLVM Verifier Errors

| Error | Cause |
|-------|-------|
| `PHI node entries do not match predecessors` | Missing phi entry for a block |
| `Instruction does not dominate all uses` | Variable used before definition in dominance order |
| `dso_local and external linkage don't match` | Setting dso_local on externally-defined globals |
| `expected instruction opcode` | Wrong IR type used in CreateXxx call |

### Quick Checks

1. **Alloca in wrong block**: All allocas must be in entry block
2. **GEP wrong indices**: Verify field indices match struct layout
3. **Type mismatch**: Check `CreateCall` arg types match function signature
4. **Phi missing incoming**: Every predecessor must have an incoming value
5. **Break/continue in wrong context**: Must be inside a loop

## Performance Considerations

1. **Type cache**: `LLVMBackendContext` caches Chemical-to-LLVM type conversions — avoid recomputing
2. **Function cache**: Generated function declarations are cached for extern functions
3. **Alloca hoisting**: All allocas in entry block → better LLVM optimization
4. **GEP simplification**: Use `CreateStructGEP` instead of manual index computation
5. **memcpy vs load+store**: LLVM can optimize memcpy; use it for struct copies
6. **Avoid redundant casts**: `CreateBitCast` is cheap but avoid chains of them
